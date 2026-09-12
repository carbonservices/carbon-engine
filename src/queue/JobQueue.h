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
             QObject* parent = nullptr);

    void start(int pollIntervalMs = 2000);

private slots:
    void pollNextJob();

private:
    std::shared_ptr<Database> m_db;
    std::shared_ptr<DocumentProcessor> m_proc;
    std::shared_ptr<LlamaEngine> m_llama;
    std::shared_ptr<VectorIndex> m_index;
    QTimer m_timer;
    bool m_isProcessing = false;
};