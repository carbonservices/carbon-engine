#pragma once
#include <QObject>
#include <QHttpServer>
#include <memory>
#include "db/Database.h"
#include "inference/LlamaEngine.h"
#include "vector/VectorIndex.h"

class HttpServer : public QObject {
    Q_OBJECT
public:
    HttpServer(std::shared_ptr<Database> db,
               std::shared_ptr<LlamaEngine> llama,
               std::shared_ptr<VectorIndex> index,
               QObject* parent = nullptr);

    bool listen(quint16 port = 8080);

private:
    QHttpServer m_server;
    std::shared_ptr<Database> m_db;
    std::shared_ptr<LlamaEngine> m_llama;
    std::shared_ptr<VectorIndex> m_index;
};