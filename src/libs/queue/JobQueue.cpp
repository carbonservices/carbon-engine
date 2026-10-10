#include "JobQueue.h"

// Qt
#include <QThreadPool>
#include <unordered_map>

JobQueue::JobQueue(Database &db, DocumentProcessor &proc, LlamaEngine &llama, VectorIndexRegistry &indexRegistry, QObject *parent)
    : QObject(parent), _db(db), _proc(proc), _llama(llama), _indexRegistry(indexRegistry)
{
    connect(&_timer, &QTimer::timeout, this, &JobQueue::pollNextJob);
}

void JobQueue::start(int pollIntervalMs)
{
    _timer.start(pollIntervalMs);
}

void JobQueue::notifyNewJob()
{
    pollNextJob();
}

void JobQueue::cancelJob(int jobId)
{
    if (_currentProcessingJobId.load() == jobId) {
        _log.warn(QString("Cancellation requested for active job %1").arg(jobId));
        _cancelRequested.store(true);
    }
}

void JobQueue::pollNextJob()
{
    if (_isProcessing.exchange(true))
        return;

    int jobId = -1;
    QVector<JobDocument> documents;
    if (!_db.getNextPendingJob(jobId, documents))
    {
        _isProcessing = false;
        return;
    }

    _currentProcessingJobId.store(jobId);
    _cancelRequested.store(false);

    _db.updateJobStatus(jobId, "PROCESSING");
    _log.info(QString("Job %1 PROCESSING: %2 document(s)").arg(jobId).arg(documents.size()));

    QThreadPool::globalInstance()->start([this, jobId, documents]()
    {
        bool inTx = false;
        try
        {
            if (_cancelRequested.load()) {
                throw std::runtime_error("Job cancelled by user");
            }

            if (!_llama.loadEmbdModel()) {
                throw std::runtime_error("Failed to load embedding model");
            }

            std::unordered_map<std::string, std::vector<float>> embeddingCache;
            std::vector<ChunkRecord> preparedChunks;

            for (const JobDocument& document : documents) {
                if (_cancelRequested.load()) {
                    throw std::runtime_error("Job cancelled by user");
                }

                auto chunks = _proc.processPdf(document.filePath.toStdString());
                for (const auto& chunk : chunks) {
                    if (_cancelRequested.load()) {
                        throw std::runtime_error("Job cancelled by user");
                    }

                    std::vector<float> emb;
                    auto it = embeddingCache.find(chunk.text);
                    if (it != embeddingCache.end()) {
                        emb = it->second;
                    } else {
                        emb = _llama.generateEmbedding(chunk.text);
                        if (!emb.empty()) {
                            embeddingCache.emplace(chunk.text, emb);
                        }
                    }

                    if (!emb.empty()) {
                        preparedChunks.push_back({
                            document.id,
                            chunk.pageNumber,
                            QString::fromStdString(chunk.text),
                            std::move(emb)
                        });
                    }
                }
            }

            // Keep embedding model resident in RAM (~67MB) for fast future ingestion & retrieval
            if (_cancelRequested.load()) {
                throw std::runtime_error("Job cancelled by user");
            }

            // Commit all prepared chunks and index vectors in a rapid, sub-millisecond transaction
            _db.transaction();
            inTx = true;
            QVector<int64_t> chunkIds = _db.insertChunksBatch(jobId, preparedChunks);
            if (chunkIds.size() != static_cast<qsizetype>(preparedChunks.size()) || !_db.commit()) {
                throw std::runtime_error("Failed to commit chunk batch transaction");
            }
            inTx = false;

            // Bulk vectorized addition into FAISS index
            std::vector<int64_t> validIds;
            std::vector<float> flatEmbeddings;
            validIds.reserve(preparedChunks.size());
            flatEmbeddings.reserve(preparedChunks.size() * 384);

            for (size_t i = 0; i < preparedChunks.size(); ++i) {
                int64_t cid = chunkIds[static_cast<qsizetype>(i)];
                if (cid > 0 && !preparedChunks[i].embedding.empty()) {
                    validIds.push_back(cid);
                    flatEmbeddings.insert(flatEmbeddings.end(),
                                          preparedChunks[i].embedding.begin(),
                                          preparedChunks[i].embedding.end());
                }
            }

            if (!validIds.empty()) {
                _indexRegistry.getOrCreate(jobId).addBatch(validIds, flatEmbeddings);
            }

            _db.updateJobStatus(jobId, "COMPLETED");
            _log.info(QString("Job %1 COMPLETED (%2 chunks, %3 unique embeddings)")
                .arg(jobId).arg(preparedChunks.size()).arg(embeddingCache.size()));
        }
        catch (const std::exception& e)
        {
            _llama.unloadEmbdModel();
            if (inTx) {
                _db.rollback();
            }
            _db.updateJobStatus(jobId, "FAILED", e.what());
            _log.error(QString("Job %1 FAILED: %2").arg(jobId).arg(e.what()));
        }

        _currentProcessingJobId.store(-1);
        _cancelRequested.store(false);
        _isProcessing = false;

        // Immediately check if there are further pending jobs in the queue
        pollNextJob();
    });
}