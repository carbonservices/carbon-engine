#include "JobQueue.h"
#include <QtConcurrent>

namespace {
// Vector ids are derived from (jobId, chunkIndex) so no separate id
// allocator is needed. This bounds chunk count per job to under 1,000,000 --
// comfortably above any realistic document -- without a DB-side sequence.
constexpr int64_t kMaxChunksPerJob = 1'000'000;
}

JobQueue::JobQueue(Database& db,
                   DocumentProcessor& proc,
                   LlamaEngine& llama,
                   VectorIndex& index,
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
    if (!_db.getNextPendingJob(jobId, filePath)) return;

    _isProcessing = true;
    _db.updateJobStatus(jobId, "PROCESSING");
    _log.info(QString("Job %1 PROCESSING: %2").arg(jobId).arg(filePath));

    QtConcurrent::run([this, jobId, filePath]() {
        try {
            auto chunks = _proc.processPdf(filePath.toStdString());
            for (size_t i = 0; i < chunks.size(); ++i) {
                auto emb = _llama.generateEmbedding(chunks[i].text);
                if (!emb.empty()) {
                    int64_t vectorId = static_cast<int64_t>(jobId) * kMaxChunksPerJob + static_cast<int64_t>(i);
                    _index.add(vectorId, emb);
                    _db.insertChunk(jobId, chunks[i].pageNumber,
                                    QString::fromStdString(chunks[i].text), vectorId);
                }
            }
            _db.updateJobStatus(jobId, "COMPLETED");
            _log.info(QString("Job %1 COMPLETED (%2 chunks)").arg(jobId).arg(chunks.size()));
        } catch (const std::exception& e) {
            _db.updateJobStatus(jobId, "FAILED", e.what());
            _log.error(QString("Job %1 FAILED: %2").arg(jobId).arg(e.what()));
        }
        _isProcessing = false;
    });
}
