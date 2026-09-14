#include "db/Database.h"
#include "inference/LlamaEngine.h"
#include "vector/VectorIndex.h"
#include "ingestion/DocumentProcessor.h"
#include "queue/JobQueue.h"
#include "api/HTTPServer.h"

// C++
#include <iostream>

// Qt
#include <QCoreApplication>
#include <QProcessEnvironment>

// dimensions for default bge-small-en-v1.5 model
const int kEmbeddingDim(384);
const quint16 kDefaultPort(8080);
const QString kDefaultDBName("ce-database");
const QString kGenerationModel("models/qwen2.5-7b-instruct-q4_k_m.gguf");
const QString kEmbeddedModel("models/bge-small-en-v1.5-f16.gguf");


QString defaultEnvironment(const QString& name, const QString& fallback)
{
    return QProcessEnvironment::systemEnvironment().value(name, fallback);
}
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    auto db = std::make_shared<Database>(kDefaultDBName);
    if (!db->init()) return -1;

    auto llama = std::make_shared<LlamaEngine>(
        defaultEnvironment("CARBON_ENGINE_GEN_MODEL", kGenerationModel),
        defaultEnvironment("CARBON_ENGINE_EMBD_MODEL", kEmbeddedModel);
    if (!llama->loadModels()) {
        std::cerr << "Failed to initialize Llama models.\n";
        return -1;
    }

    auto vectorIndex = std::make_shared<VectorIndex>(kEmbeddingDim);
    auto docProcessor = std::make_shared<DocumentProcessor>();

    auto jobQueue = std::make_unique<JobQueue>(db, docProcessor, llama, vectorIndex);
    jobQueue->start(1500);

    auto server = std::make_unique<HttpServer>(db, llama, vectorIndex);
    if (!server->listen(kDefaultPort))
    {
        std::cerr << "Failed to bind HTTP server to port " << kDefaultPort << ".\n";
        return -1;
    }

    std::cout << "Carbon Engine listening on http://localhost:" << kDefaultPort << "\n";
    return app.exec();
}
