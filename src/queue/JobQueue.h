#pragma once
#include <QObject>
#include <QTimer>
#include <memory>
#include "db/Database.h"
#include "ingestion/DocumentProcessor.h"
#include "inference/LlamaEngine.h"
#include "vector/VectorIndex.h"

class JobQueue : public QObject {
    Q_OBJECT
public:
    JobQueue(std::shared_ptr<Database> db,
             std::shared_ptr<DocumentProcessor> proc,
             std::shared_ptr<LlamaEngine> llama,
             std::shared_ptr<VectorIndex> index,
             QObject* parent = Q_NULLPTR);

    void start(int pollIntervalMs = 2000);

private slots:
    void pollNextJob();

private:
    std::shared_ptr<Database> _db;
    std::shared_ptr<DocumentProcessor> _proc;
    std::shared_ptr<LlamaEngine> _llama;
    std::shared_ptr<VectorIndex> _index;
    QTimer _timer;
    bool _isProcessing = false;
};
