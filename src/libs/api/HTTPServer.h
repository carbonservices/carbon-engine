#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "db/Database.h"
#include "inference/LlamaEngine.h"
#include "vector/VectorIndex.h"
#include "logging/Logger.h"

// Qt
#include <QObject>
#include <QHttpServer>

// Owns the QHttpServer and the four /api/* routes. Database/LlamaEngine/
// VectorIndex are owned by main() for the process lifetime and outlive every
// request, so routes hold plain references rather than shared_ptr.
class HttpServer : public QObject {
    Q_OBJECT
public:
    HttpServer(Database& db, LlamaEngine& llama, VectorIndex& index, QObject* parent = Q_NULLPTR);

    bool listen(quint16 port = 8080);

private:
    Database& _db;
    LlamaEngine& _llama;
    VectorIndex& _index;
    QHttpServer _server;
    Logger _log{"HttpServer"};
};

#endif // HTTPSERVER_H
