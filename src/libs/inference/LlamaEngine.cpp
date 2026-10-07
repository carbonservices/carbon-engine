#include "LlamaEngine.h"

#include <QMutexLocker>

// C++ standard
#include <thread>
#include <chrono>

constexpr int kContextSize        = 4096;
constexpr int kMaxGeneratedTokens = 512;

// ─────────────────────────────────────────────────────────────
// Clamp hardware concurrency to a safe range: floor=2, ceil=16.
// ─────────────────────────────────────────────────────────────
static int resolveThreadCount()
{
    unsigned hw = std::thread::hardware_concurrency();
    if (hw == 0) hw = 4;
    if (hw < 2)  hw = 2;
    if (hw > 16) hw = 16;
    return static_cast<int>(hw);
}

// ─────────────────────────────────────────────────────────────
LlamaEngine::LlamaEngine(const QString& genModelPath,
                         const QString& embdModelPath,
                         SystemMonitor& monitor)
    : _genPath(genModelPath.toStdString())
    , _embdPath(embdModelPath.toStdString())
    , _nThreads(resolveThreadCount())
    , _monitor(monitor)
{}

LlamaEngine::~LlamaEngine()
{
    unloadGenModel();
    unloadEmbdModel();
    llama_backend_free();
}

// ─────────────────────────────────────────────────────────────
bool LlamaEngine::initBackend()
{
    llama_backend_init();
    _log.info(QString("LlamaEngine: backend initialised, using %1 threads").arg(_nThreads));
    return true;
}

// ─────────────────────────────────────────────────────────────
// Embedding model
// ─────────────────────────────────────────────────────────────
bool LlamaEngine::loadEmbdModel()
{
    QMutexLocker lock(&_embdMutex);
    if (_embdModel) {
        return true;
    }

    _monitor.setEnginePhase(EnginePhase::LoadingModel);
    _log.info("Loading embedding model...");

    llama_model_params mp = llama_model_default_params();
    _embdModel = llama_model_load_from_file(_embdPath.c_str(), mp);
    if (!_embdModel) {
        _log.error(QString("Failed to load embedding model: %1")
                       .arg(QString::fromStdString(_embdPath)));
        _monitor.setEnginePhase(EnginePhase::Idle);
        return false;
    }

    if (!initEmbdContext()) {
        llama_model_free(_embdModel);
        _embdModel = Q_NULLPTR;
        _monitor.setEnginePhase(EnginePhase::Idle);
        return false;
    }

    _log.info("Embedding model loaded");
    _monitor.setEnginePhase(EnginePhase::Idle);
    return true;
}

bool LlamaEngine::initEmbdContext()
{
    llama_context_params cp = llama_context_default_params();
    cp.n_ctx           = kContextSize;
    cp.n_batch         = kContextSize;
    cp.n_ubatch        = kContextSize;
    cp.n_threads       = _nThreads;
    cp.n_threads_batch = _nThreads;
    cp.embeddings      = true;
    cp.pooling_type    = LLAMA_POOLING_TYPE_MEAN;

    _embdCtx = llama_init_from_model(_embdModel, cp);
    if (!_embdCtx) {
        _log.error("Failed to create embedding context");
        return false;
    }
    return true;
}

void LlamaEngine::unloadEmbdModel()
{
    QMutexLocker lock(&_embdMutex);
    if (_embdCtx) {
        llama_free(_embdCtx);
        _embdCtx = Q_NULLPTR;
    }
    if (_embdModel) {
        llama_model_free(_embdModel);
        _embdModel = Q_NULLPTR;
        _log.info("Embedding model unloaded");
    }
}

// ─────────────────────────────────────────────────────────────
// Generation model
// ─────────────────────────────────────────────────────────────
bool LlamaEngine::loadGenModel()
{
    QMutexLocker lock(&_genMutex);
    if (_genModel) {
        return true;
    }

    _monitor.setEnginePhase(EnginePhase::LoadingModel);
    _log.info("Loading generation model...");

    llama_model_params mp = llama_model_default_params();
    _genModel = llama_model_load_from_file(_genPath.c_str(), mp);
    if (!_genModel) {
        _log.error(QString("Failed to load generation model: %1")
                       .arg(QString::fromStdString(_genPath)));
        _monitor.setEnginePhase(EnginePhase::Idle);
        return false;
    }

    if (!initGenContext()) {
        llama_model_free(_genModel);
        _genModel = Q_NULLPTR;
        _monitor.setEnginePhase(EnginePhase::Idle);
        return false;
    }

    _log.info("Generation model loaded");
    _monitor.setEnginePhase(EnginePhase::Idle);
    return true;
}

bool LlamaEngine::initGenContext()
{
    llama_context_params cp = llama_context_default_params();
    cp.n_ctx           = kContextSize;
    cp.n_threads       = _nThreads;
    cp.n_batch         = kContextSize;
    cp.n_ubatch        = kContextSize;
    cp.n_threads_batch = _nThreads;

    _genCtx = llama_init_from_model(_genModel, cp);
    if (!_genCtx) {
        _log.error("Failed to create generation context");
        return false;
    }

    // Sampler chain
    llama_sampler_chain_params sp = llama_sampler_chain_default_params();
    _sampler = llama_sampler_chain_init(sp);

    const llama_vocab* vocab = llama_model_get_vocab(_genModel);
    int32_t n_vocab = llama_vocab_n_tokens(vocab);

    llama_sampler_chain_add(_sampler, llama_sampler_init_penalties(n_vocab, 64, 1.15f, 0.10f, 0.10f));
    llama_sampler_chain_add(_sampler, llama_sampler_init_top_k(40));
    llama_sampler_chain_add(_sampler, llama_sampler_init_top_p(0.9f, 1));
    llama_sampler_chain_add(_sampler, llama_sampler_init_temp(0.7f));
    llama_sampler_chain_add(_sampler, llama_sampler_init_dist(0));

    return true;
}

void LlamaEngine::unloadGenModel()
{
    QMutexLocker lock(&_genMutex);
    if (_sampler) {
        llama_sampler_free(_sampler);
        _sampler = Q_NULLPTR;
    }
    if (_genCtx) {
        llama_free(_genCtx);
        _genCtx = Q_NULLPTR;
    }
    if (_genModel) {
        llama_model_free(_genModel);
        _genModel = Q_NULLPTR;
        _log.info("Generation model unloaded");
    }
}

// ─────────────────────────────────────────────────────────────
// Inference
// ─────────────────────────────────────────────────────────────
std::vector<float> LlamaEngine::generateEmbedding(const std::string& text)
{
    QMutexLocker locker(&_embdMutex);

    if (!_embdModel || !_embdCtx) {
        _log.error("generateEmbedding: embedding model not loaded");
        return {};
    }

    _monitor.setEnginePhase(EnginePhase::Embedding);

    const llama_vocab* vocab = llama_model_get_vocab(_embdModel);
    std::vector<llama_token> tokens(text.size() + 8);
    int nTokens = llama_tokenize(vocab, text.c_str(), static_cast<int32_t>(text.length()),
                                 tokens.data(), static_cast<int32_t>(tokens.size()), true, false);
    if (nTokens < 0) {
        _monitor.setEnginePhase(EnginePhase::Idle);
        return {};
    }
    tokens.resize(nTokens);

    llama_memory_clear(llama_get_memory(_embdCtx), true);

    llama_batch batch = llama_batch_get_one(tokens.data(), static_cast<int32_t>(tokens.size()));
    if (llama_decode(_embdCtx, batch) != 0) {
        _monitor.setEnginePhase(EnginePhase::Idle);
        return {};
    }

    int nEmbd = llama_model_n_embd(_embdModel);
    const float* embeddings = llama_get_embeddings_seq(_embdCtx, 0);
    if (!embeddings) {
        _monitor.setEnginePhase(EnginePhase::Idle);
        return {};
    }

    std::vector<float> result(embeddings, embeddings + nEmbd);
    _monitor.setEnginePhase(EnginePhase::Idle);
    return result;
}

void LlamaEngine::streamCompletion(const std::string& prompt,
                                   std::function<bool(const std::string&)> tokenCallback)
{
    QMutexLocker locker(&_genMutex);

    if (!_genModel || !_genCtx) {
        _log.error("streamCompletion: generation model not loaded");
        return;
    }

    _monitor.setEnginePhase(EnginePhase::Inferencing);

    const llama_vocab* vocab = llama_model_get_vocab(_genModel);
    std::vector<llama_token> tokens(prompt.size() + 8);
    int nTokens = llama_tokenize(vocab, prompt.c_str(), static_cast<int32_t>(prompt.length()),
                                 tokens.data(), static_cast<int32_t>(tokens.size()), true, true);
    if (nTokens < 0) {
        _monitor.setEnginePhase(EnginePhase::Idle);
        return;
    }
    tokens.resize(nTokens);

    // Ensure prompt + max generated tokens fits comfortably in the context window
    if (tokens.size() + kMaxGeneratedTokens > static_cast<size_t>(kContextSize)) {
        tokens.resize(kContextSize - kMaxGeneratedTokens);
    }

    llama_memory_clear(llama_get_memory(_genCtx), true);
    llama_sampler_reset(_sampler);

    uint32_t n_batch = llama_n_batch(_genCtx);
    llama_batch batch = llama_batch_init(n_batch, 0, 1);

    // Ingest prompt in batches
    for (size_t i = 0; i < tokens.size(); i += n_batch) {
        int n_eval = std::min(static_cast<int>(n_batch),
                              static_cast<int>(tokens.size() - i));
        batch.n_tokens = 0;
        for (int j = 0; j < n_eval; j++) {
            bool isLast = (i + j == tokens.size() - 1);
            batch.token[batch.n_tokens]       = tokens[i + j];
            batch.pos[batch.n_tokens]         = static_cast<llama_pos>(i + j);
            batch.n_seq_id[batch.n_tokens]    = 1;
            batch.seq_id[batch.n_tokens][0]   = 0;
            batch.logits[batch.n_tokens]      = isLast;
            batch.n_tokens++;
        }
        if (llama_decode(_genCtx, batch) != 0) {
            llama_batch_free(batch);
            _monitor.setEnginePhase(EnginePhase::Idle);
            return;
        }
    }

    // Autoregressive generation loop — measure tokens/sec
    auto tStart = std::chrono::steady_clock::now();
    int tokensGenerated = 0;

    for (int i = 0; i < kMaxGeneratedTokens; ++i) {
        llama_token nextToken = llama_sampler_sample(_sampler, _genCtx, -1);
        llama_sampler_accept(_sampler, nextToken);

        if (llama_vocab_is_eog(vocab, nextToken))
            break;

        char buf[128];
        int n = llama_token_to_piece(vocab, nextToken, buf, sizeof(buf), 0, true);
        if (n > 0) {
            std::string piece(buf, n);
            if (piece.find("<|im_end|>") != std::string::npos ||
                piece.find("<|endoftext|>") != std::string::npos) {
                break;
            }
            if (!tokenCallback(piece)) {
                break;
            }
        }

        ++tokensGenerated;

        batch.n_tokens = 0;
        batch.token[batch.n_tokens]     = nextToken;
        batch.pos[batch.n_tokens]       = static_cast<llama_pos>(tokens.size() + i);
        batch.n_seq_id[batch.n_tokens]  = 1;
        batch.seq_id[batch.n_tokens][0] = 0;
        batch.logits[batch.n_tokens]    = true;
        batch.n_tokens++;

        if (llama_decode(_genCtx, batch) != 0)
            break;
    }

    llama_batch_free(batch);

    // Publish tokens/sec
    auto elapsed = std::chrono::steady_clock::now() - tStart;
    double secs  = std::chrono::duration<double>(elapsed).count();
    if (secs > 0.0 && tokensGenerated > 0)
        _monitor.setTokensPerSec(static_cast<double>(tokensGenerated) / secs);

    _monitor.setEnginePhase(EnginePhase::Idle);
}
