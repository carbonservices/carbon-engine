#include "ServerConfig.h"

#include "api/HTTPServer.h"
#include "db/Database.h"
#include "inference/LlamaEngine.h"
#include "ingestion/DocumentProcessor.h"
#include "logging/Logger.h"
#include "queue/JobQueue.h"
#include "report/BrsrSchema.h"
#include "report/ReportExtractionService.h"
#include "report/ReportExporter.h"
#include "system/SystemMonitor.h"
#include "vector/VectorIndexRegistry.h"

#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("Carbon Engine Service");
    app.setApplicationVersion("1.0.0");
    Logger log("Server-Main");

    Database db(kDatabaseFilePath);
    if (!db.init())
        return -1;

    SystemMonitor monitor;

    LlamaEngine llama(kGenerationModelPath, kEmbeddingModelPath, monitor);
    if (!llama.initBackend())
    {
        log.critical("Failed to initialize Llama backend.");
        return -1;
    }

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&llama, &log]() {
        log.info("Server shutting down, releasing inference resources...");
        llama.shutdown();
    });

    VectorIndexRegistry indexRegistry(kEmbeddingDim, &db);
    DocumentProcessor docProcessor;

    JobQueue jobQueue(db, docProcessor, llama, indexRegistry);
    jobQueue.start(1500);

    BrsrSchemaCatalog schemaCatalog;
    ReportExtractionService extractionService(db, llama, indexRegistry, schemaCatalog, monitor);
    ReportExporter exporter(db, schemaCatalog);

    HttpServer server(db, llama, indexRegistry, monitor, schemaCatalog, extractionService, exporter, jobQueue);
    if (!server.listen(kDefaultPort))
    {
        log.critical(QString("Failed to bind HTTP server to port %1.").arg(kDefaultPort));
        return -1;
    }

    log.info(QString("Carbon Engine listening on http://localhost:%1").arg(kDefaultPort));
    return app.exec();
}
