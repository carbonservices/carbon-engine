#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

#include "db/Database.h"
#include "ingestion/DocumentProcessor.h"
#include "inference/LlamaEngine.h"
#include "logging/Logger.h"
#include "vector/VectorIndex.h"

// Qt
#include <QObject>
#include <QTimer>

// C++
#include <atomic>


class JobQueue : public QObject
{
    Q_OBJECT
public:
    explicit JobQueue(Database& db, DocumentProcessor& proc, LlamaEngine& llama, VectorIndex& index, QObject* parent = Q_NULLPTR);
    void start(int pollIntervalMs = 2000);

private slots:
    void pollNextJob();

private:
    static constexpr int64_t MaxChunksPerJob = 1'000'000;

    Database& _db;
    DocumentProcessor& _proc;
    LlamaEngine& _llama;
    VectorIndex& _index;
    QTimer _timer;
    std::atomic<bool> _isProcessing{false};
    Logger _log{"JobQueue"};
};

#endif