#include <QCoreApplication>
#include <iostream>
#include "db/Database.h"
#include "inference/LlamaEngine.h"
#include "vector/VectorIndex.h"
#include "ingestion/DocumentProcessor.h"
#include "queue/JobQueue.h"
#include "api/HttpServer.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    auto db = std::make_shared<Database>("esg_local.db");
    if (!db->init()) return -1;

    // Load Quantized GGUF models (7B Generative + Small Embedding model)
    auto llama = std::make_shared<LlamaEngine>("models/qwen2.5-7b-instruct-q4_k_m.gguf",
                                               "models/bge-small-en-v1.5-f16.gguf");
    if (!llama->loadModels()) {
        std::cerr << "Failed to initialize Llama models.\n";
        return -1;
    }

    auto vectorIndex = std::make_shared<VectorIndex>(384); // 384 dimensions for bge-small
    auto docProcessor = std::make_shared<DocumentProcessor>();

    auto jobQueue = std::make_unique<JobQueue>(db, docProcessor, llama, vectorIndex);
    jobQueue->start(1500);

    auto server = std::make_unique<HttpServer>(db, llama, vectorIndex);
    if (!server->listen(8080)) {
        std::cerr << "Failed to bind HTTP server to port 8080.\n";
        return -1;
    }

    std::cout << "ESG Local C++ Core listening on http://localhost:8080\n";
    return app.exec();
}