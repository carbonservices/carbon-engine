#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

#include "db/Database.h"
#include "ingestion/DocumentProcessor.h"
#include "inference/LlamaEngine.h"
#include "logging/Logger.h"
#include "vector/VectorIndexRegistry.h"

#include <QObject>
#include <QTimer>
#include <atomic>

class JobQueue : public QObject
{
    Q_OBJECT
public:
    explicit JobQueue(Database& db, DocumentProcessor& proc, LlamaEngine& llama, VectorIndexRegistry& indexRegistry, QObject* parent = Q_NULLPTR);
    void start(int pollIntervalMs = 2000);

    void notifyNewJob();
    void cancelJob(int jobId);

    bool isProcessing() const { return _isProcessing.load(); }
    int currentJobId() const { return _currentProcessingJobId.load(); }

signals:
    void jobCompleted(int jobId);

public slots:
    void pollNextJob();

    void executeJob(int jobId, const QVector<JobDocument>& documents);

private:
    Database& _db;
    DocumentProcessor& _proc;
    LlamaEngine& _llama;
    VectorIndexRegistry& _indexRegistry;
    QTimer _timer;
    std::atomic<bool> _isProcessing{false};
    std::atomic<int> _currentProcessingJobId{CoreConstants::NoActiveJobId};
    std::atomic<bool> _cancelRequested{false};

    Logger _log{"JobQueue"};
};

#endif // JOB_QUEUE_H

