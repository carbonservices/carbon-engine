#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "db/Database.h"
#include "inference/LlamaEngine.h"
#include "vector/VectorIndex.h"

// C++
#include <memory>

// Qt
#include <QObject>
#include <QHttpServer>


class ApiRoute {
public:
    virtual ~ApiRoute() = default;
    virtual QString path() const = 0;
    virtual QHttpServerRequest::Method method() const = 0;
    virtual QFuture<QHttpServerResponse> handle(const QHttpServerRequest& request) = 0;
};

class IngestRoute : public ApiRoute {
public:
    explicit IngestRoute(std::shared_ptr<Database> db);

    QString path() const override;
    QHttpServerRequest::Method method() const override;
    QFuture<QHttpServerResponse> handle(const QHttpServerRequest& request) override;

private:
    std::shared_ptr<Database> _db;
};

class ChatRoute : public ApiRoute {
public:
    ChatRoute(std::shared_ptr<LlamaEngine> llama, std::shared_ptr<VectorIndex> index);

    QString path() const override;
    QHttpServerRequest::Method method() const override;
    QFuture<QHttpServerResponse> handle(const QHttpServerRequest& request) override;

private:
    std::shared_ptr<LlamaEngine> _llama;
    std::shared_ptr<VectorIndex> _index;
};

class HttpServer : public QObject {
    Q_OBJECT
public:
    HttpServer(std::shared_ptr<Database> db,
               std::shared_ptr<LlamaEngine> llama,
               std::shared_ptr<VectorIndex> index,
               QObject* parent = Q_NULLPTR);

    bool listen(quint16 port = 8080);

private:
    void registerRoute(std::unique_ptr<ApiRoute> route);

    QHttpServer _server;
    std::vector<std::unique_ptr<ApiRoute>> _routes;
};

#endif // HTTPSERVER_H
