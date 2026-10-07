#include "JobQueue.h"

// Qt
#include <QThreadPool>

JobQueue::JobQueue(Database &db, DocumentProcessor &proc, LlamaEngine &llama, VectorIndex &index, QObject *parent)
    : QObject(parent), _db(db), _proc(proc), _llama(llama), _index(index)
{
    connect(&_timer, &QTimer::timeout, this, &JobQueue::pollNextJob);
}

void JobQueue::start(int pollIntervalMs)
{
    _timer.start(pollIntervalMs);
}

void JobQueue::pollNextJob()
{
    if (_isProcessing.exchange(true))
        return;

    int jobId = -1;
    QString filePath;
    if (!_db.getNextPendingJob(jobId, filePath))
    {
        _isProcessing = false;
        return;
    }

    _db.updateJobStatus(jobId, "PROCESSING");
    _log.info(QString("Job %1 PROCESSING: %2").arg(jobId).arg(filePath));

    QThreadPool::globalInstance()->start([this, jobId, filePath]()
    {
        try
        {
            auto chunks = _proc.processPdf(filePath.toStdString());

            if (!_llama.loadEmbdModel()) {
                throw std::runtime_error("Failed to load embedding model");
            }

            _db.transaction();
            for (size_t i = 0; i < chunks.size(); ++i) {
                auto emb = _llama.generateEmbedding(chunks[i].text);
                if (!emb.empty()) {
                    int64_t vectorId = static_cast<int64_t>(jobId) * MaxChunksPerJob + static_cast<int64_t>(i);
                    _index.add(vectorId, emb);
                    _db.insertChunk(jobId, chunks[i].pageNumber,
                                    QString::fromStdString(chunks[i].text), vectorId);
                }
            }
            _db.commit();

            _llama.unloadEmbdModel();

            _db.updateJobStatus(jobId, "COMPLETED");
            _log.info(QString("Job %1 COMPLETED (%2 chunks)").arg(jobId).arg(chunks.size()));
        }
        catch (const std::exception& e)
        {
            _llama.unloadEmbdModel();
            _db.rollback();
            _db.updateJobStatus(jobId, "FAILED", e.what());
            _log.error(QString("Job %1 FAILED: %2").arg(jobId).arg(e.what()));
        }

        _isProcessing = false;
    });
}