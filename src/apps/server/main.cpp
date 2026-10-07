#include "ServerConfig.h"

#include "api/HTTPServer.h"
#include "db/Database.h"
#include "inference/LlamaEngine.h"
#include "ingestion/DocumentProcessor.h"
#include "logging/Logger.h"
#include "queue/JobQueue.h"
#include "system/SystemMonitor.h"
#include "vector/VectorIndex.h"

#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
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

    VectorIndex vectorIndex(kEmbeddingDim);
    DocumentProcessor docProcessor;

    JobQueue jobQueue(db, docProcessor, llama, vectorIndex);
    jobQueue.start(1500);

    HttpServer server(db, llama, vectorIndex, monitor);
    if (!server.listen(kDefaultPort))
    {
        log.critical(QString("Failed to bind HTTP server to port %1.").arg(kDefaultPort));
        return -1;
    }

    log.info(QString("Carbon Engine listening on http://localhost:%1").arg(kDefaultPort));
    return app.exec();
}
