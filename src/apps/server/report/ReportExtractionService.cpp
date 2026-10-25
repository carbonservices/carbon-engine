#include "ReportExtractionService.h"
#include "ServerConfig.h"
#include "constants/CommonConstants.h"
#include "report/JsonRepairEngine.h"

// Qt
#include <QFutureSynchronizer>
#include <QtConcurrent>
#include <nlohmann/json.hpp>
#include <unordered_set>

#include <QThreadPool>
#include <QRunnable>

class AsyncExtractionTask : public QRunnable {
public:
    AsyncExtractionTask(ReportExtractionService* service, int jobId)
        : _service(service), _jobId(jobId) {}

    void run() override {
        if (_service) {
            _service->extractJob(_jobId);
        }
    }

private:
    ReportExtractionService* _service;
    int _jobId;
};

struct JsonStreamTokenCollector {
    std::atomic<bool>& cancelRequested;
    bool& firstToken;
    qint64& prefillTime;
    const QElapsedTimer& genTimer;
    std::string& generatedJson;
    int& tokenCount;
    int braceDepth{1};
    bool inString{false};
    bool escapeNext{false};

    bool operator()(const std::string& token) {
        if (cancelRequested.load()) {
            return false;
        }

        if (firstToken) {
            prefillTime = genTimer.elapsed();
            firstToken = false;
        }

        generatedJson += token;
        tokenCount++;

        // Track balanced braces to stop right after root JSON closing brace
        for (char c : token) {
            if (escapeNext) {
                escapeNext = false;
                continue;
            }
            if (c == '\\') {
                escapeNext = true;
                continue;
            }
            if (c == '"') {
                inString = !inString;
                continue;
            }
            if (!inString) {
                if (c == '{') braceDepth++;
                else if (c == '}') {
                    braceDepth--;
                    if (braceDepth <= 0) {
                        return false;
                    }
                }
            }
        }
        return true;
    }
};

ReportExtractionService::ReportExtractionService(Database& db, LlamaEngine& llama,
                                                 VectorIndexRegistry& indexRegistry,
                                                 const BRSRSchemaCatalog& catalog,
                                                 SystemMonitor& monitor,
                                                 QObject* parent)
    : QObject(parent), _db(db), _llama(llama), _indexRegistry(indexRegistry), _catalog(catalog), _monitor(monitor), _factStore(&db)
{
}

void ReportExtractionService::startAsyncExtraction(int jobId)
{
    _log.info(QString("Starting asynchronous statutory extraction for job %1").arg(jobId));
    QThreadPool::globalInstance()->start(new AsyncExtractionTask(this, jobId));
}

ReportExtractionService::GroupContext ReportExtractionService::retrieveContextForGroup(
    int jobId, const FieldGroup& group)
{
    if (_cancelRequested.load()) {
        return {group.id, QString(), QVector<int64_t>(), 0};
    }

    QElapsedTimer retTimer;
    retTimer.start();

    std::vector<int64_t> allIds;
    std::unordered_set<int64_t> seenVectorIds;

    // 1. Vector Similarity Search first (FAISS HNSW k=6) - primary semantic relevance
    std::vector<float> queryEmbedding = _llama.generateEmbedding(group.ragQuery.toStdString());

    if (!queryEmbedding.empty()) {
        auto index = _indexRegistry.getOrCreate(jobId);
        std::vector<int64_t> vectorIds = index ? index->search(queryEmbedding, 6) : std::vector<int64_t>{};
        for (int64_t vectorId : vectorIds) {
            if (vectorId >= 0 && seenVectorIds.insert(vectorId).second) {
                allIds.push_back(vectorId);
            }
        }
    }

    if (_cancelRequested.load()) {
        return {group.id, QString(), QVector<int64_t>(), 0};
    }

    // 2. Locality-anchored chunks from FactStore (topic anchors + immediate +/- 1 neighbors)
    QVector<int64_t> anchorIds = _factStore.getAnchorChunksForTopic(jobId, group.section);
    for (int64_t anchorId : anchorIds) {
        if (anchorId > 0) {
            for (int64_t delta = -1; delta <= 1; ++delta) {
                int64_t neighborId = anchorId + delta;
                if (neighborId > 0 && seenVectorIds.insert(neighborId).second) {
                    allIds.push_back(neighborId);
                }
            }
        }
    }

    if (_cancelRequested.load()) {
        return {group.id, QString(), QVector<int64_t>(), 0};
    }

    // 3. Batched single-query SQLite lookup with strict character budget (~2,200 tokens)
    constexpr int kMaxContextChars = 9000;
    auto chunkMap = _db.getChunksByIds(allIds);
    QString assembledContext;
    QVector<int64_t> matchedIds;

    for (int64_t id : allIds) {
        auto it = chunkMap.find(id);
        if (it != chunkMap.end()) {
            if (assembledContext.length() + it.value().length() > kMaxContextChars) {
                break;
            }
            assembledContext += "\n---\n" + it.value();
            matchedIds.push_back(id);
        }
    }

    return {group.id, assembledContext, matchedIds, retTimer.elapsed()};
}

QString ReportExtractionService::buildInstructions(int jobId, const FieldGroup& group) const
{
    QString fieldList;
    for (const QString& key : group.dataFieldKeys) {
        fieldList += "- " + key + ": a precise numeric or short factual value\n";
    }
    for (const QString& key : group.narrativeFieldKeys) {
        fieldList += "- " + key + ": a short narrative paragraph grounded only in the context below\n";
    }
    for (const QString& key : group.loopFieldKeys) {
        fieldList += "- " + key + ": " + BRSRSchemaCatalog::loopFieldHint(key) + "\n";
    }

    QString knownFacts = _factStore.formatKnownFactsForPrompt(jobId);

    QString instructions =
        "You are a strict data extraction AI for Business Responsibility and Sustainability "
        "Reporting (BRSR), currently extracting only the \"" + group.label + "\" section.\n"
        + knownFacts +
        "Extract exactly these fields from the context into a flat JSON object:\n" + fieldList +
        "\nRules:\n"
        "1. Output ONLY a valid JSON object starting with { and ending with }.\n"
        "2. If a field cannot be found in the context, set its value to null (or [] for array fields).\n"
        "3. Do NOT include markdown code fences, notes, or explanations.\n"
        "4. Narrative fields must be grounded only in the provided context, not invented.";

    return instructions;
}

std::string ReportExtractionService::buildPrompt(int jobId, const FieldGroup& group, const QString& context) const
{
    QString instructions = buildInstructions(jobId, group);
    bool isGemma = kGenerationModelPath.contains("gemma", Qt::CaseInsensitive);
    if (isGemma) {
        return "<start_of_turn>user\n" + instructions.toStdString() +
               "\n\nContext:\n" + context.toStdString() +
               "\n\nExtract JSON.<end_of_turn>\n"
               "<start_of_turn>model\n{";
    }
    return "<|im_start|>system\nYou are a strict data extraction AI. Output valid JSON only.<|im_end|>\n"
           "<|im_start|>user\n" + instructions.toStdString() +
           "\n\nContext:\n" + context.toStdString() +
           "\n\nExtract JSON.<|im_end|>\n"
           "<|im_start|>assistant\n{";
}

void ReportExtractionService::generateAndPersistGroup(int jobId, const FieldGroup& group,
                                                     const GroupContext& ctx,
                                                     qint64& outPrefillMs, qint64& outDecodeMs,
                                                     int& outTokensGenerated)
{
    QElapsedTimer genTimer;
    genTimer.start();

    std::string generatedJson = "{";
    int tokenCount = 0;
    qint64 prefillTime = 0;
    bool firstToken = true;

    {
        QMutexLocker locker(&_progressMutex);
        _progress.engineMode = EngineMode::Offline;
        _progress.activeModel = QFileInfo(kGenerationModelPath).fileName();
    }

    std::string prompt = buildPrompt(jobId, group, ctx.assembledContext);

    GenerationOptions options;
    options.temperature = 0.0f;
    options.maxTokens = group.loopFieldKeys.isEmpty() ? 768 : 1536;
    options.repeatPenalty = 1.05f;

    int braceDepth = 1;
    bool inString = false;
    bool escapeNext = false;

    _monitor.setEnginePhase(EnginePhase::Inferencing);

    JsonStreamTokenCollector collector{_cancelRequested, firstToken, prefillTime, genTimer, generatedJson, tokenCount};
    _llama.streamCompletion(prompt, std::ref(collector), options);

    _monitor.setEnginePhase(EnginePhase::Idle);

    if (_cancelRequested.load()) {
        return;
    }

    outPrefillMs = prefillTime;
    outDecodeMs = genTimer.elapsed() - prefillTime;
    outTokensGenerated = tokenCount;

    nlohmann::json parsed = JsonRepairEngine::parseOrRecoverJson(generatedJson);
    if (parsed.is_null() || !parsed.is_object()) {
        _log.error(QString("Failed to salvage JSON for group %1").arg(group.id));
        parsed = nlohmann::json::object();
    }

    std::string dumpStr = parsed.dump();
    QString sectionJson = QString::fromStdString(dumpStr);

    QStringList sourceIdStrings;
    for (int64_t id : ctx.sourceChunkIds) {
        sourceIdStrings.append(QString::number(id));
    }
    QString sourceChunksStr = sourceIdStrings.join(",");

    _db.upsertReportSection(jobId, group.id, sectionJson, ReportSectionStatus::Draft, sourceChunksStr);

    int nonNullFacts = 0;
    int64_t primaryChunkId = ctx.sourceChunkIds.isEmpty() ? -1 : ctx.sourceChunkIds.first();

    for (auto it = parsed.begin(); it != parsed.end(); ++it) {
        if (!it.value().is_null()) {
            std::string valStr;
            if (it.value().is_string()) {
                valStr = it.value().get<std::string>();
            } else if (it.value().is_number()) {
                valStr = it.value().dump();
            }
            if (!valStr.empty() && valStr != "null" && valStr != "N/A") {
                _factStore.put(jobId, QString::fromStdString(it.key()), QString::fromStdString(valStr), primaryChunkId);
                nonNullFacts++;
            }
        }
    }

    if (nonNullFacts >= 2 && primaryChunkId > 0) {
        _factStore.recordTopicAnchor(jobId, group.section, primaryChunkId, 0);
    }

    QStringList missing;
    for (const auto& key : group.allFieldKeys()) {
        std::string k = key.toStdString();
        if (!parsed.contains(k) || parsed[k].is_null()) {
            missing.append(key);
        }
    }

    if (!missing.isEmpty()) {
        QMutexLocker locker(&_progressMutex);
        _progress.missingSections.append({group.id, group.label, missing});
    }

    _log.info(QString("Persisted section %1 (%2 fields, %3 missing, decode %4ms)")
        .arg(group.id).arg(group.allFieldKeys().size()).arg(missing.size()).arg(outDecodeMs));
}

void ReportExtractionService::cancelJob(int jobId)
{
    if (_activeJobId.load() == jobId) {
        _log.warn(QString("Extraction cancellation requested for job %1").arg(jobId));
        _cancelRequested.store(true);
    }
}

void ReportExtractionService::extractJob(int jobId)
{
    if (_isExtracting.exchange(true)) {
        _log.warn(QString("Cannot start extraction for job %1: job %2 is already extracting")
                      .arg(jobId).arg(_activeJobId.load()));
        return;
    }

    _activeJobId.store(jobId);
    _cancelRequested.store(false);

    struct ExtractionGuard {
        std::atomic<bool>& isExtracting;
        std::atomic<int>& activeJobId;
        std::atomic<bool>& cancelRequested;
        ~ExtractionGuard() {
            activeJobId.store(-1);
            cancelRequested.store(false);
            isExtracting.store(false);
        }
    } guard{_isExtracting, _activeJobId, _cancelRequested};

    try {
        QElapsedTimer totalJobTimer;
        totalJobTimer.start();

        _db.setExtractionStatus(jobId, ExtractionStatus::Extracting);
        _log.info(QString("Starting high-precision per-section extraction for job %1").arg(jobId));

        {
            QMutexLocker locker(&_progressMutex);
            _progress.currentStep = 0;
            _progress.totalSteps = _catalog.groups().size();
            _progress.currentGroupId = "";
            _progress.currentGroupLabel = "Retrieving vector contexts...";
            _progress.missingSections.clear();
        }

        if (_cancelRequested.load()) {
            _db.setExtractionStatus(jobId, ExtractionStatus::Cancelled);
            _log.warn(QString("Job %1 extraction cancelled before start").arg(jobId));
            return;
        }

        // 1. Initialize and load any persisted facts for this job
        _factStore.loadForJob(jobId);

        // 2. Context retrieval phase: embedding model query and FAISS lookup
        if (!_llama.loadEmbdModel()) {
            _db.setExtractionStatus(jobId, ExtractionStatus::Failed);
            _log.error(QString("Job %1: failed to load embedding model").arg(jobId));
            return;
        }

        QVector<GroupContext> contexts;
        contexts.reserve(_catalog.groups().size());
        for (const auto& group : _catalog.groups()) {
            if (_cancelRequested.load()) break;
            contexts.append(retrieveContextForGroup(jobId, group));
        }

        if (_cancelRequested.load()) {
            _db.setExtractionStatus(jobId, ExtractionStatus::Cancelled);
            _log.warn(QString("Job %1 extraction cancelled during retrieval").arg(jobId));
            return;
        }

        // Swap out embedding model to reclaim RAM/VRAM before loading generative model
        _llama.unloadEmbdModel();

        if (!_llama.loadGenModel()) {
            _db.setExtractionStatus(jobId, ExtractionStatus::Failed);
            _log.error(QString("Job %1: failed to load generation model").arg(jobId));
            return;
        }

        // 3. Sequential extraction of each disclosure group
        int step = 0;
        for (int i = 0; i < _catalog.groups().size(); ++i) {
            if (_cancelRequested.load()) {
                _db.setExtractionStatus(jobId, ExtractionStatus::Cancelled);
                _log.warn(QString("Job %1 extraction cancelled at step %2").arg(jobId).arg(step));
                return;
            }

            const auto& group = _catalog.groups()[i];
            const auto& ctx = contexts[i];
            step++;

            {
                QMutexLocker locker(&_progressMutex);
                _progress.currentStep = step;
                _progress.totalSteps = _catalog.groups().size();
                _progress.currentGroupId = group.id;
                _progress.currentGroupLabel = group.label;
            }

            qint64 prefillMs = 0, decodeMs = 0;
            int tokensGen = 0;
            generateAndPersistGroup(jobId, group, ctx, prefillMs, decodeMs, tokensGen);

            {
                QMutexLocker locker(&_progressMutex);
                _progress.lastLatencyMs = prefillMs + decodeMs;
            }
        }

        if (_cancelRequested.load()) {
            _db.setExtractionStatus(jobId, ExtractionStatus::Cancelled);
            return;
        }

        _db.setExtractionStatus(jobId, ExtractionStatus::ReadyForReview);
        _log.info(QString("Job %1 statutory extraction COMPLETE in %2s -> READY_FOR_REVIEW")
            .arg(jobId).arg(totalJobTimer.elapsed() / 1000.0, 0, 'f', 1));

        _llama.scheduleIdleUnload(300);
    } catch (const std::exception& e) {
        _log.error(QString("Job %1 extraction crashed: %2").arg(jobId).arg(e.what()));
        _db.setExtractionStatus(jobId, ExtractionStatus::Failed);
    }
}

ReportExtractionService::ProgressInfo ReportExtractionService::currentProgress() const
{
    QMutexLocker locker(&_progressMutex);
    return _progress;
}

