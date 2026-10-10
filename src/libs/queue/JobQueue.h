#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

#include "db/Database.h"
#include "ingestion/DocumentProcessor.h"
#include "inference/LlamaEngine.h"
#include "logging/Logger.h"
#include "vector/VectorIndexRegistry.h"

// Qt
#include <QObject>
#include <QTimer>

// C++
#include <atomic>


class JobQueue : public QObject
{
    Q_OBJECT
public:
    explicit JobQueue(Database& db, DocumentProcessor& proc, LlamaEngine& llama, VectorIndexRegistry& indexRegistry, QObject* parent = Q_NULLPTR);
    void start(int pollIntervalMs = 2000);

    // Event-driven immediate wakeup when a new job is enqueued
    void notifyNewJob();

    // Cooperative cancellation if a job is deleted or aborted
    void cancelJob(int jobId);

    bool isProcessing() const { return _isProcessing.load(); }

public slots:
    void pollNextJob();

private:
    Database& _db;
    DocumentProcessor& _proc;
    LlamaEngine& _llama;
    VectorIndexRegistry& _indexRegistry;
    QTimer _timer;
    std::atomic<bool> _isProcessing{false};
    std::atomic<int> _currentProcessingJobId{-1};
    std::atomic<bool> _cancelRequested{false};
    Logger _log{"JobQueue"};
};

#endif