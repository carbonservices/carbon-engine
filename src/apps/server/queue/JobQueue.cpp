#include "JobQueue.h"
#include "constants/CommonConstants.h"

// Qt
#include <QThreadPool>
#include <unordered_map>

#include <QRunnable>

class JobWorkerTask : public QRunnable {
public:
    JobWorkerTask(JobQueue* queue, int jobId, QVector<JobDocument> documents)
        : _queue(queue), _jobId(jobId), _documents(std::move(documents)) {}

    void run() override {
        if (_queue) {
            try {
                _queue->executeJob(_jobId, _documents);
            } catch (const std::exception&) {
                // Precautionary guard: executeJob handles its own exceptions
            } catch (...) {
                // Precautionary guard against non-standard exceptions
            }
        }
    }

private:
    JobQueue* _queue;
    int _jobId;
    QVector<JobDocument> _documents;
};

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

    _db.updateJobStatus(jobId, JobStatus::Processing);
    for (const auto& doc : documents) {
        _db.updateDocumentStatus(doc.id, "QUEUED");
    }
    _log.info(QString("Job %1 PROCESSING: %2 document(s)").arg(jobId).arg(documents.size()));

    QThreadPool::globalInstance()->start(new JobWorkerTask(this, jobId, documents));
}

void JobQueue::executeJob(int jobId, const QVector<JobDocument>& documents)
{
        bool inTx = false;
        try
        {
            if (_cancelRequested.load()) {
                throw std::runtime_error("Job cancelled by user");
            }

            if (!_llama.loadEmbdModel()) {
                throw std::runtime_error("Failed to load local embedding model");
            }

            _log.info(QString("Job %1: Generating vector embeddings via Local BGE (bge-small-en-v1.5) for %2 document(s)...")
                .arg(jobId).arg(documents.size()));

            std::unordered_map<std::string, std::vector<float>> embeddingCache;
            std::vector<ChunkRecord> preparedChunks;

            for (const JobDocument& document : documents) {
                if (_cancelRequested.load()) {
                    break;
                }

                _db.updateDocumentStatus(document.id, "PROCESSING");
                try {
                    auto chunks = _proc.processPdf(document.filePath.toStdString());
                    for (const auto& chunk : chunks) {
                        if (_cancelRequested.load()) {
                            break;
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
                    _db.updateDocumentStatus(document.id, "INGESTED");
                } catch (const std::exception& docEx) {
                    _log.error(QString("Failed to process document %1 (%2): %3")
                        .arg(document.id).arg(document.filePath).arg(docEx.what()));
                    _db.updateDocumentStatus(document.id, "FAILED");
                }
            }

            if (_cancelRequested.load()) {
                throw std::runtime_error("Job cancelled by user");
            }

            // Commit all prepared chunks and index vectors in an atomic transaction
            _db.transaction();
            inTx = true;
            QVector<int64_t> chunkIds = _db.insertChunksBatch(jobId, preparedChunks, "bge-small-en-v1.5");
            if (chunkIds.size() != static_cast<qsizetype>(preparedChunks.size()) || !_db.commit()) {
                throw std::runtime_error("Failed to commit chunk batch transaction");
            }
            inTx = false;

            // Bulk vectorized addition into FAISS index
            std::vector<int64_t> validIds;
            std::vector<float> flatEmbeddings;
            validIds.reserve(preparedChunks.size());
            flatEmbeddings.reserve(preparedChunks.size() * ServerDefaults::DefaultEmbeddingDim);

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
                auto index = _indexRegistry.getOrCreate(jobId);
                if (index) {
                    index->addBatch(validIds, flatEmbeddings);
                }
            }

            _db.updateJobStatus(jobId, JobStatus::Completed);
            _log.info(QString("Job %1 COMPLETED (%2 chunks, %3 unique embeddings)")
                .arg(jobId).arg(preparedChunks.size()).arg(embeddingCache.size()));

            emit jobCompleted(jobId);
        }
        catch (const std::exception& e)
        {
            if (inTx) {
                _db.rollback();
            }
            QString finalStatus = _cancelRequested.load() ? JobStatus::Cancelled : JobStatus::Failed;
            _db.updateJobStatus(jobId, finalStatus, e.what());
            for (const auto& doc : documents) {
                _db.updateDocumentStatus(doc.id, _cancelRequested.load() ? "CANCELLED" : "FAILED");
            }
            if (_cancelRequested.load()) {
                _log.warn(QString("Job %1 CANCELLED").arg(jobId));
            } else {
                _log.error(QString("Job %1 FAILED: %2").arg(jobId).arg(e.what()));
            }
        }
        catch (...)
        {
            if (inTx) {
                _db.rollback();
            }
            QString finalStatus = _cancelRequested.load() ? JobStatus::Cancelled : JobStatus::Failed;
            _db.updateJobStatus(jobId, finalStatus, "Unknown internal error");
            for (const auto& doc : documents) {
                _db.updateDocumentStatus(doc.id, _cancelRequested.load() ? "CANCELLED" : "FAILED");
            }
            _log.error(QString("Job %1 FAILED: Unknown internal exception").arg(jobId));
        }

        _currentProcessingJobId.store(-1);
        _cancelRequested.store(false);
        _isProcessing = false;

        // Check for subsequent jobs
        pollNextJob();
}

