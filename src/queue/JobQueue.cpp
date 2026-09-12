// src/queue/JobQueue.cpp
#include "JobQueue.h"
#include <QtConcurrent>

JobQueue::JobQueue(std::shared_ptr<Database> db,
                   std::shared_ptr<DocumentProcessor> proc,
                   std::shared_ptr<LlamaEngine> llama,
                   std::shared_ptr<VectorIndex> index,
                   QObject* parent)
    : QObject(parent), m_db(db), m_proc(proc), m_llama(llama), m_index(index) {
    connect(&m_timer, &QTimer::timeout, this, &JobQueue::pollNextJob);
}

void JobQueue::start(int pollIntervalMs) {
    m_timer.start(pollIntervalMs);
}

void JobQueue::pollNextJob() {
    if (m_isProcessing) return;

    int jobId = -1;
    QString filePath;
    if (!m_db->getNextPendingJob(jobId, filePath)) return;

    m_isProcessing = true;
    m_db->updateJobStatus(jobId, "PROCESSING");

    // Run extraction, chunking, and embedding off the network loop
    QtConcurrent::run([this, jobId, filePath]() {
        try {
            auto chunks = m_proc->processPdf(filePath.toStdString());
            for (size_t i = 0; i < chunks.size(); ++i) {
                auto emb = m_llama->generateEmbedding(chunks[i].text);
                if (!emb.empty()) {
                    int64_t vectorId = static_cast<int64_t>(jobId) * 10000 + i;
                    m_index->add(vectorId, emb);
                }
            }
            m_db->updateJobStatus(jobId, "COMPLETED");
        } catch (const std::exception& e) {
            m_db->updateJobStatus(jobId, "FAILED", e.what());
        }
        m_isProcessing = false;
    });
}