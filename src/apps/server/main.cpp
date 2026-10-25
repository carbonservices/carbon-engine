#include "ServerConfig.h"
#include "api/HTTPServer.h"
#include "db/Database.h"
#include "inference/LlamaEngine.h"
#include "ingestion/DocumentProcessor.h"
#include "logging/Logger.h"
#include "queue/JobQueue.h"
#include "report/BRSRSchema.h"
#include "report/ReportExtractionService.h"
#include "report/ReportExporter.h"
#include "system/SystemMonitor.h"
#include "vector/VectorIndexRegistry.h"
#include "system/SingleInstanceGuard.h"
#include "system/WindowsService.h"

#include <QCoreApplication>
#include <iostream>

int runServerDaemon(int argc, char *argv[])
{
    SingleInstanceGuard guard(AppInfo::DaemonMutexName);
    if (!guard.isPrimary())
    {
        Logger log(AppInfo::ServerAppName);
        log.critical(QString("Another instance of %1 is already running. Exiting.").arg(AppInfo::ServerAppName));
        std::cerr << "Another instance of " << AppInfo::ServerAppName << " is already running. Exiting." << std::endl;
        return 1;
    }

    QCoreApplication app(argc, argv);
    app.setApplicationName(AppInfo::ServerAppName);
    app.setOrganizationName(AppInfo::OrganizationName);
    app.setOrganizationDomain(AppInfo::OrganizationDomain);
    app.setApplicationVersion(AppInfo::Version);
    Logger log(AppInfo::ServerAppName);

    Database db(kDatabaseFilePath);
    if (!db.init())
    {
        log.critical("Database initialization failed. Exiting.");
        return 1;
    }

    SystemMonitor monitor;
    LlamaEngine llama(kGenerationModelPath, kEmbeddingModelPath, monitor);
    if (!llama.initBackend())
    {
        log.critical("llama.cpp backend initialization failed. Exiting.");
        return 1;
    }

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&llama]() {
        llama.shutdown();
    });

    VectorIndexRegistry indexRegistry(kEmbeddingDim, &db);
    DocumentProcessor docProcessor;

    JobQueue jobQueue(db, docProcessor, llama, indexRegistry);
    jobQueue.start(1500);

    BRSRSchemaCatalog schemaCatalog;
    ReportExtractionService extractionService(db, llama, indexRegistry, schemaCatalog, monitor);
    ReportExporter exporter(db, schemaCatalog);

    QObject::connect(&jobQueue, &JobQueue::jobCompleted, [&extractionService, &log](int jobId) {
        log.info(QString("Job %1 ingestion complete. Automatically advancing to statutory extraction on backend...").arg(jobId));
        QThreadPool::globalInstance()->start([&extractionService, jobId]() {
            extractionService.extractJob(jobId);
        });
    });

    HttpServer server(db, llama, indexRegistry, monitor, schemaCatalog, extractionService, exporter, jobQueue);
    if (!server.listen(kDefaultPort))
    {
        log.critical(QString("Failed to bind HTTP server to port %1.").arg(kDefaultPort));
        return 1;
    }

    log.info(QString("Carbon Engine listening on http://localhost:%1").arg(kDefaultPort));
    return app.exec();
}

int main(int argc, char *argv[])
{
    if (WindowsService::handleServiceCommands(argc, argv, &runServerDaemon))
    {
        return 0;
    }
    return runServerDaemon(argc, argv);
}
