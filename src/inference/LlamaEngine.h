#pragma once
#include <string>
#include <vector>
#include <functional>
#include "llama.h"

class LlamaEngine {
public:
    LlamaEngine(const std::string& genModelPath, const std::string& embdModelPath);
    ~LlamaEngine();

    bool loadModels();
    std::vector<float> generateEmbedding(const std::string& text);
    void streamCompletion(const std::string& prompt, std::function<void(const std::string&)> tokenCallback);

private:
    std::string m_genPath;
    std::string m_embdPath;
    llama_model* m_genModel = nullptr;
    llama_context* m_genCtx = nullptr;
    llama_model* m_embdModel = nullptr;
    llama_context* m_embdCtx = nullptr;
};