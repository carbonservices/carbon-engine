#include "LlamaEngine.h"

#include <iostream>
#include <QMutexLocker>

constexpr int kContextSize = 4096;
constexpr int kThreads = 4;
constexpr int kMaxGeneratedTokens = 512;

LlamaEngine::LlamaEngine(const std::string& genModelPath, const std::string& embdModelPath)
    : _genPath(genModelPath), _embdPath(embdModelPath) {}

LlamaEngine::~LlamaEngine() {
    if (_sampler) llama_sampler_free(_sampler);
    if (_genCtx) llama_free(_genCtx);
    if (_genModel) llama_model_free(_genModel);
    if (_embdCtx) llama_free(_embdCtx);
    if (_embdModel) llama_model_free(_embdModel);
    llama_backend_free();
}

bool LlamaEngine::loadModels() {
    llama_backend_init();

    llama_model_params modelParams = llama_model_default_params();
    _genModel = llama_model_load_from_file(_genPath.c_str(), modelParams);
    if (!_genModel) {
        std::cerr << "Failed to load generation model: " << _genPath << "\n";
        return false;
    }

    llama_context_params ctxParams = llama_context_default_params();
    ctxParams.n_ctx = kContextSize;
    ctxParams.n_threads = kThreads;
    ctxParams.n_threads_batch = kThreads;
    _genCtx = llama_init_from_model(_genModel, ctxParams);
    if (!_genCtx) {
        std::cerr << "Failed to create generation context\n";
        return false;
    }

    llama_sampler_chain_params samplerParams = llama_sampler_chain_default_params();
    _sampler = llama_sampler_chain_init(samplerParams);
    llama_sampler_chain_add(_sampler, llama_sampler_init_top_k(40));
    llama_sampler_chain_add(_sampler, llama_sampler_init_top_p(0.9f, 1));
    llama_sampler_chain_add(_sampler, llama_sampler_init_temp(0.7f));
    llama_sampler_chain_add(_sampler, llama_sampler_init_dist(0));

    _embdModel = llama_model_load_from_file(_embdPath.c_str(), modelParams);
    if (!_embdModel) {
        std::cerr << "Failed to load embedding model: " << _embdPath << "\n";
        return false;
    }

    llama_context_params embdCtxParams = llama_context_default_params();
    embdCtxParams.n_ctx = kContextSize;
    embdCtxParams.n_threads = kThreads;
    embdCtxParams.n_threads_batch = kThreads;
    embdCtxParams.embeddings = true;
    embdCtxParams.pooling_type = LLAMA_POOLING_TYPE_MEAN;
    _embdCtx = llama_init_from_model(_embdModel, embdCtxParams);
    if (!_embdCtx) {
        std::cerr << "Failed to create embedding context\n";
        return false;
    }

    return true;
}

std::vector<float> LlamaEngine::generateEmbedding(const std::string& text) {
    QMutexLocker locker(&_embdMutex);

    const llama_vocab* vocab = llama_model_get_vocab(_embdModel);
    std::vector<llama_token> tokens(text.size() + 8);
    int nTokens = llama_tokenize(vocab, text.c_str(), static_cast<int32_t>(text.length()),
                                  tokens.data(), static_cast<int32_t>(tokens.size()), true, false);
    if (nTokens < 0) return {};
    tokens.resize(nTokens);

    llama_memory_clear(llama_get_memory(_embdCtx), true);

    llama_batch batch = llama_batch_get_one(tokens.data(), static_cast<int32_t>(tokens.size()));
    if (llama_decode(_embdCtx, batch) != 0) return {};

    int nEmbd = llama_model_n_embd(_embdModel);
    const float* embeddings = llama_get_embeddings_seq(_embdCtx, 0);
    if (!embeddings) return {};

    return std::vector<float>(embeddings, embeddings + nEmbd);
}

void LlamaEngine::streamCompletion(const std::string& prompt, std::function<void(const std::string&)> tokenCallback) {
    QMutexLocker locker(&_genMutex);

    const llama_vocab* vocab = llama_model_get_vocab(_genModel);
    std::vector<llama_token> tokens(prompt.size() + 8);
    int nTokens = llama_tokenize(vocab, prompt.c_str(), static_cast<int32_t>(prompt.length()),
                                 tokens.data(), static_cast<int32_t>(tokens.size()), true, true);
    if (nTokens < 0) return;
    tokens.resize(nTokens);

    llama_memory_clear(llama_get_memory(_genCtx), true);
    llama_sampler_reset(_sampler);

    llama_batch batch = llama_batch_get_one(tokens.data(), static_cast<int32_t>(tokens.size()));
    if (llama_decode(_genCtx, batch) != 0) return;

    for (int i = 0; i < kMaxGeneratedTokens; ++i) {
        llama_token nextToken = llama_sampler_sample(_sampler, _genCtx, -1);
        llama_sampler_accept(_sampler, nextToken);

        if (llama_vocab_is_eog(vocab, nextToken)) break;

        char buf[128];
        int n = llama_token_to_piece(vocab, nextToken, buf, sizeof(buf), 0, false);
        if (n > 0) {
            tokenCallback(std::string(buf, n));
        }

        llama_batch nextBatch = llama_batch_get_one(&nextToken, 1);
        if (llama_decode(_genCtx, nextBatch) != 0) break;
    }
}
