#pragma once
#include <string>
#include <vector>
#include <functional>
#include <QMutex>
#include <QtGlobal>
#include "llama.h"
#include "logging/Logger.h"
#include "system/SystemMonitor.h"

class LlamaEngine {
public:
    LlamaEngine(const QString& genModelPath,
                const QString& embdModelPath,
                SystemMonitor& monitor);
    ~LlamaEngine();

    // ── Granular load/unload ──────────────────────────────────
    // Called once at startup to init the llama backend.
    bool initBackend();

    // Load / unload the embedding model on demand.
    bool loadEmbdModel();
    void unloadEmbdModel();

    // Load / unload the generation model on demand.
    bool loadGenModel();
    void unloadGenModel();

    // ── Inference ─────────────────────────────────────────────
    std::vector<float> generateEmbedding(const std::string& text);
    void streamCompletion(const std::string& prompt,
                          std::function<bool(const std::string&)> tokenCallback);

private:
    bool initGenContext();
    bool initEmbdContext();

    std::string _genPath;
    std::string _embdPath;
    int         _nThreads = 4;

    llama_model*   _genModel  = Q_NULLPTR;
    llama_context* _genCtx    = Q_NULLPTR;
    llama_model*   _embdModel = Q_NULLPTR;
    llama_context* _embdCtx   = Q_NULLPTR;
    llama_sampler* _sampler   = Q_NULLPTR;

    QMutex _genMutex;
    QMutex _embdMutex;

    SystemMonitor& _monitor;
    Logger _log{"LlamaEngine"};
};
