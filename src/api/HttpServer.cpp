#include "HttpServer.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QtConcurrent>
#include <QTcpServer>
#include <QHostAddress>

IngestRoute::IngestRoute(std::shared_ptr<Database> db) : _db(db) {}

QString IngestRoute::path() const { return "/api/ingest"; }
QHttpServerRequest::Method IngestRoute::method() const { return QHttpServerRequest::Method::Post; }

QFuture<QHttpServerResponse> IngestRoute::handle(const QHttpServerRequest& request) {
    auto doc = QJsonDocument::fromJson(request.body());
    QString filePath = doc.object().value("file_path").toString();
    auto db = _db;

    return QtConcurrent::run([db, filePath]() {
        if (filePath.isEmpty() || !db->enqueueJob(filePath)) {
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
        }
        return QHttpServerResponse(QJsonObject{{"status", "QUEUED"}});
    });
}

ChatRoute::ChatRoute(std::shared_ptr<LlamaEngine> llama, std::shared_ptr<VectorIndex> index)
    : _llama(llama), _index(index) {}

QString ChatRoute::path() const { return "/api/chat"; }
QHttpServerRequest::Method ChatRoute::method() const { return QHttpServerRequest::Method::Post; }

QFuture<QHttpServerResponse> ChatRoute::handle(const QHttpServerRequest& request) {
    auto doc = QJsonDocument::fromJson(request.body());
    QString prompt = doc.object().value("prompt").toString();
    auto llama = _llama;
    auto index = _index;

    return QtConcurrent::run([llama, index, prompt]() {
        if (prompt.isEmpty()) {
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
        }

        std::string promptStd = prompt.toStdString();
        auto queryEmbedding = llama->generateEmbedding(promptStd);
        auto matchedIds = index->search(queryEmbedding, 3);

        std::string generatedOutput;
        llama->streamCompletion(promptStd, [&generatedOutput](const std::string& token) {
            generatedOutput += token;
        });

        return QHttpServerResponse(QJsonObject{
            {"response", QString::fromStdString(generatedOutput)},
            {"context_matches", static_cast<int>(matchedIds.size())}
        });
    });
}

HttpServer::HttpServer(std::shared_ptr<Database> db,
                       std::shared_ptr<LlamaEngine> llama,
                       std::shared_ptr<VectorIndex> index,
                       QObject* parent)
    : QObject(parent) {
    registerRoute(std::make_unique<IngestRoute>(db));
    registerRoute(std::make_unique<ChatRoute>(llama, index));
}

void HttpServer::registerRoute(std::unique_ptr<ApiRoute> route) {
    ApiRoute* rawRoute = route.get();
    _server.route(rawRoute->path(), rawRoute->method(),
        [rawRoute](const QHttpServerRequest& request) {
            return rawRoute->handle(request);
        });
    _routes.push_back(std::move(route));
}

bool HttpServer::listen(quint16 port) {
    auto* tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, port)) return false;
    return _server.bind(tcpServer);
}
