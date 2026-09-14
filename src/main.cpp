#include <QCoreApplication>
#include <iostream>
#include <cstdlib>
#include "db/Database.h"
#include "inference/LlamaEngine.h"
#include "vector/VectorIndex.h"
#include "ingestion/DocumentProcessor.h"
#include "queue/JobQueue.h"
#include "api/HttpServer.h"

constexpr int kEmbeddingDim = 384; // bge-small-en-v1.5
constexpr quint16 kDefaultPort = 8080;

std::string envOr(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    return value ? std::string(value) : fallback;
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    auto db = std::make_shared<Database>("esg_local.db");
    if (!db->init()) return -1;

    auto llama = std::make_shared<LlamaEngine>(
        envOr("CARBON_ENGINE_GEN_MODEL", "models/qwen2.5-7b-instruct-q4_k_m.gguf"),
        envOr("CARBON_ENGINE_EMBD_MODEL", "models/bge-small-en-v1.5-f16.gguf"));
    if (!llama->loadModels()) {
        std::cerr << "Failed to initialize Llama models.\n";
        return -1;
    }

    auto vectorIndex = std::make_shared<VectorIndex>(kEmbeddingDim);
    auto docProcessor = std::make_shared<DocumentProcessor>();

    auto jobQueue = std::make_unique<JobQueue>(db, docProcessor, llama, vectorIndex);
    jobQueue->start(1500);

    auto server = std::make_unique<HttpServer>(db, llama, vectorIndex);
    if (!server->listen(kDefaultPort)) {
        std::cerr << "Failed to bind HTTP server to port " << kDefaultPort << ".\n";
        return -1;
    }

    std::cout << "carbon-engine listening on http://localhost:" << kDefaultPort << "\n";
    return app.exec();
}
