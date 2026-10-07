#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "db/Database.h"
#include "inference/LlamaEngine.h"
#include "logging/Logger.h"
#include "system/SystemMonitor.h"
#include "vector/VectorIndex.h"

// Qt
#include <QObject>
#include <QHttpServer>
#include <QFuture>


class HttpServer : public QObject
{
    Q_OBJECT

public:
    explicit HttpServer(Database& db, LlamaEngine& llama, VectorIndex& index,
                        SystemMonitor& monitor, QObject* parent = Q_NULLPTR);
    bool listen(quint16 port);

private:
    QFuture<QHttpServerResponse> handleIngest(const QHttpServerRequest& request);
    QFuture<QHttpServerResponse> handleChat(const QHttpServerRequest& request);
    QFuture<QHttpServerResponse> handleReport(const QHttpServerRequest& request);
    QFuture<QHttpServerResponse> handleJobStatus(const QHttpServerRequest& request);
    QFuture<QHttpServerResponse> handleSystemStatus(const QHttpServerRequest& request);

    QHttpServer             _server;
    Database&               _db;
    LlamaEngine&            _llama;
    VectorIndex&            _index;
    SystemMonitor&          _monitor;
    QMutex                  _reportCacheMutex;
    QMap<int, QString>      _cachedReports;
    Logger                  _log{"HttpServer"};
};

#endif // HTTPSERVER_H

