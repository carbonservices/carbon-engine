#include "JobQueue.h"
#include <QtConcurrent>

JobQueue::JobQueue(std::shared_ptr<Database> db,
                   std::shared_ptr<DocumentProcessor> proc,
                   std::shared_ptr<LlamaEngine> llama,
                   std::shared_ptr<VectorIndex> index,
                   QObject* parent)
    : QObject(parent), _db(db), _proc(proc), _llama(llama), _index(index) {
    connect(&_timer, &QTimer::timeout, this, &JobQueue::pollNextJob);
}

void JobQueue::start(int pollIntervalMs) {
    _timer.start(pollIntervalMs);
}

void JobQueue::pollNextJob() {
    if (_isProcessing) return;

    int jobId = -1;
    QString filePath;
    if (!_db->getNextPendingJob(jobId, filePath)) return;

    _isProcessing = true;
    _db->updateJobStatus(jobId, "PROCESSING");

    QtConcurrent::run([this, jobId, filePath]() {
        try {
            auto chunks = _proc->processPdf(filePath.toStdString());
            for (size_t i = 0; i < chunks.size(); ++i) {
                auto emb = _llama->generateEmbedding(chunks[i].text);
                if (!emb.empty()) {
                    int64_t vectorId = static_cast<int64_t>(jobId) * 10000 + static_cast<int64_t>(i);
                    _index->add(vectorId, emb);
                    _db->insertChunk(jobId, chunks[i].pageNumber,
                                     QString::fromStdString(chunks[i].text), vectorId);
                }
            }
            _db->updateJobStatus(jobId, "COMPLETED");
        } catch (const std::exception& e) {
            _db->updateJobStatus(jobId, "FAILED", e.what());
        }
        _isProcessing = false;
    });
}
