#pragma once
#include <QObject>
#include <QTimer>
#include "db/Database.h"
#include "ingestion/DocumentProcessor.h"
#include "inference/LlamaEngine.h"
#include "vector/VectorIndex.h"
#include "logging/Logger.h"

// Polls Database for pending ingestion jobs and runs them (extract -> embed
// -> index) on a QtConcurrent worker. All dependencies are owned by main()
// for the process lifetime, so plain references are enough here.
class JobQueue : public QObject {
    Q_OBJECT
public:
    JobQueue(Database& db,
             DocumentProcessor& proc,
             LlamaEngine& llama,
             VectorIndex& index,
             QObject* parent = Q_NULLPTR);

    void start(int pollIntervalMs = 2000);

private slots:
    void pollNextJob();

private:
    Database& _db;
    DocumentProcessor& _proc;
    LlamaEngine& _llama;
    VectorIndex& _index;
    QTimer _timer;
    bool _isProcessing = false;
    Logger _log{"JobQueue"};
};
