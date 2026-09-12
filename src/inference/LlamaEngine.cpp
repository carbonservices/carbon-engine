#include "LlamaEngine.h"

#include <iostream>

LlamaEngine::LlamaEngine(const std::string& genModelPath, const std::string& embdModelPath)
    : m_genPath(genModelPath), m_embdPath(embdModelPath) {}

LlamaEngine::~LlamaEngine() {
    if (m_genCtx) llama_free(m_genCtx);
    if (m_genModel) llama_free_model(m_genModel);
    if (m_embdCtx) llama_free(m_embdCtx);
    if (m_embdModel) llama_free_model(m_embdModel);
    llama_backend_free();
}

bool LlamaEngine::loadModels() {
    llama_backend_init();

    // 1. Load Generation Model
    llama_model_params model_params = llama_model_default_params();
    m_genModel = llama_load_model_from_file(m_genPath.c_str(), model_params);
    if (!m_genModel) return false;

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 2048;
    ctx_params.n_threads = 4;
    m_genCtx = llama_new_context_with_model(m_genModel, ctx_params);

    // 2. Load Embedding Model
    llama_context_params embd_ctx_params = llama_context_default_params();
    embd_ctx_params.embeddings = true;
    embd_ctx_params.n_threads = 4;

    m_embdModel = llama_load_model_from_file(m_embdPath.c_str(), model_params);
    if (!m_embdModel) return false;

    m_embdCtx = llama_new_context_with_model(m_embdModel, embd_ctx_params);
    return m_genCtx && m_embdCtx;
}

std::vector<float> LlamaEngine::generateEmbedding(const std::string& text) {
    std::vector<llama_token> tokens(text.size() + 1);
    int n_tokens = llama_tokenize(m_embdModel, text.c_str(), text.length(), tokens.data(), tokens.size(), true, false);
    if (n_tokens < 0) return {};
    tokens.resize(n_tokens);

    llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
    if (llama_decode(m_embdCtx, batch) != 0) return {};

    int n_embd = llama_n_embd(m_embdModel);
    const float* embeddings = llama_get_embeddings(m_embdCtx);
    if (!embeddings) return {};

    return std::vector<float>(embeddings, embeddings + n_embd);
}

void LlamaEngine::streamCompletion(const std::string& prompt, std::function<void(const std::string&)> tokenCallback) {
    std::vector<llama_token> tokens(prompt.size() + 1);
    int n_tokens = llama_tokenize(m_genModel, prompt.c_str(), prompt.length(), tokens.data(), tokens.size(), true, false);
    tokens.resize(n_tokens);

    llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
    llama_decode(m_genCtx, batch);

    for (int i = 0; i < 256; ++i) { // max token generation loop
        llama_token next_token = llama_sampler_sample(nullptr, m_genCtx, -1);
        if (llama_token_is_eog(m_genModel, next_token)) break;

        char buf[128];
        int n = llama_token_to_piece(m_genModel, next_token, buf, sizeof(buf), 0, false);
        if (n > 0) {
            tokenCallback(std::string(buf, n));
        }

        llama_batch next_batch = llama_batch_get_one(&next_token, 1);
        if (llama_decode(m_genCtx, next_batch) != 0) break;
    }
}