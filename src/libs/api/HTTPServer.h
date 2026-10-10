#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "db/Database.h"
#include "inference/LlamaEngine.h"
#include "logging/Logger.h"
#include "queue/JobQueue.h"
#include "report/BrsrSchema.h"
#include "report/ReportExporter.h"
#include "report/ReportExtractionService.h"
#include "system/SystemMonitor.h"
#include "vector/VectorIndexRegistry.h"

// Qt
#include <QObject>
#include <QHttpServer>
#include <QFuture>


class HttpServer : public QObject
{
    Q_OBJECT

public:
    explicit HttpServer(Database& db, LlamaEngine& llama, VectorIndexRegistry& indexRegistry,
                        SystemMonitor& monitor, const BrsrSchemaCatalog& catalog,
                        ReportExtractionService& extractionService, ReportExporter& exporter,
                        JobQueue& jobQueue,
                        QObject* parent = Q_NULLPTR);
    bool listen(quint16 port);

private:
    QFuture<QHttpServerResponse> handleIngest(const QHttpServerRequest& request);
    QFuture<QHttpServerResponse> handleJobStatus(const QHttpServerRequest& request);
    QFuture<QHttpServerResponse> handleSystemStatus(const QHttpServerRequest& request);

    QFuture<QHttpServerResponse> handleExtractionStart(const QHttpServerRequest& request);
    QFuture<QHttpServerResponse> handleExtractionStatus(const QHttpServerRequest& request);
    QFuture<QHttpServerResponse> handleExtractionSections(const QHttpServerRequest& request);
    QFuture<QHttpServerResponse> handleExtractionSectionUpdate(const QHttpServerRequest& request);
    QFuture<QHttpServerResponse> handleExtractionAccept(const QHttpServerRequest& request);
    QFuture<QHttpServerResponse> handleReportExport(const QHttpServerRequest& request);
    QFuture<QHttpServerResponse> handleReportsList(const QHttpServerRequest& request);
    QFuture<QHttpServerResponse> handleReportDelete(const QHttpServerRequest& request);
    QFuture<QHttpServerResponse> handleJobDocuments(const QHttpServerRequest& request);

    QHttpServer               _server;
    Database&                 _db;
    LlamaEngine&               _llama;
    VectorIndexRegistry&       _indexRegistry;
    SystemMonitor&             _monitor;
    const BrsrSchemaCatalog&   _catalog;
    ReportExtractionService&   _extractionService;
    ReportExporter&            _exporter;
    JobQueue&                  _jobQueue;
    Logger                     _log{"HttpServer"};
};

#endif // HTTPSERVER_H

