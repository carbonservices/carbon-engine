#pragma once
#include <string>
#include <vector>
#include <functional>
#include <QMutex>
#include <QtGlobal>
#include "llama.h"
#include "logging/Logger.h"

class LlamaEngine {
public:
    LlamaEngine(const QString& genModelPath, const QString& embdModelPath);
    ~LlamaEngine();

    bool loadModels();
    std::vector<float> generateEmbedding(const std::string& text);
    void streamCompletion(const std::string& prompt, std::function<void(const std::string&)> tokenCallback);

private:
    std::string _genPath;
    std::string _embdPath;
    llama_model* _genModel = Q_NULLPTR;
    llama_context* _genCtx = Q_NULLPTR;
    llama_model* _embdModel = Q_NULLPTR;
    llama_context* _embdCtx = Q_NULLPTR;
    llama_sampler* _sampler = Q_NULLPTR;
    QMutex _genMutex;
    QMutex _embdMutex;
    Logger _log{"LlamaEngine"};
};
