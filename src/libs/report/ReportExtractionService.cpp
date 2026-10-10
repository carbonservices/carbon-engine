#include "ReportExtractionService.h"

// Qt
#include <QFutureSynchronizer>
#include <QtConcurrent>

// Templating
#include <nlohmann/json.hpp>
#include <unordered_set>

namespace {

QString getLoopFieldHint(const QString& key) {
    static const QHash<QString, QString> s_schemaHints = {
        {"business_activities", "JSON array of objects with fields [{\"description\": \"...\", \"turnover_pct\": \"...\"}]"},
        {"products_sold", "JSON array of objects with fields [{\"product_name\": \"...\", \"nic_code\": \"...\", \"turnover_pct\": \"...\"}]"},
        {"subsidiaries", "JSON array of objects with fields [{\"company_name\": \"...\", \"business_activity\": \"...\", \"holding_pct\": \"...\"}]"},
        {"csr_projects", "JSON array of objects with fields [{\"project_name\": \"...\", \"state_district\": \"...\", \"amount_spent\": \"...\"}]"},
        {"stakeholder_grievances", "JSON array of objects with fields [{\"stakeholder_group\": \"...\", \"received_current\": \"...\", \"resolved_current\": \"...\"}]"},
        {"penalties", "JSON array of objects with fields [{\"authority\": \"...\", \"amount\": \"...\", \"brief_case\": \"...\"}]"},
        {"material_issues", "JSON array of objects with fields [{\"material_issue\": \"...\", \"rationale\": \"...\", \"risk_or_opportunity\": \"...\"}]"},
        {"turnover_rates", "JSON array of objects with fields [{\"category\": \"...\", \"current_rate\": \"...\", \"prev_rate\": \"...\"}]"},
        {"principle_policy_matrix", "JSON array of objects with fields [{\"principle\": \"...\", \"policy_formulated\": \"...\", \"board_approved\": \"...\"}]"},
        {"review_by_committee", "JSON array of objects with fields [{\"subject\": \"...\", \"frequency\": \"...\", \"committee_name\": \"...\"}]"},
        {"gap_reasons", "JSON array of objects with fields [{\"principle\": \"...\", \"reason\": \"...\"}]"},
        {"p1_training", "JSON array of objects with fields [{\"category\": \"...\", \"anti_corruption_trained_pct\": \"...\"}]"},
        {"p1_fines", "JSON array of objects with fields [{\"agency\": \"...\", \"amount\": \"...\", \"brief\": \"...\"}]"},
        {"p1_concentration", "JSON array of objects with fields [{\"metric\": \"...\", \"value\": \"...\"}]"},
        {"p2_reclaim", "JSON array of objects with fields [{\"material\": \"...\", \"reclaimed_pct\": \"...\"}]"},
        {"p2_lca", "JSON array of objects with fields [{\"product_name\": \"...\", \"turnover_pct\": \"...\"}]"},
        {"p2_recycled_input", "JSON array of objects with fields [{\"material\": \"...\", \"recycled_pct_current\": \"...\"}]"},
        {"p3_wellbeing", "JSON array of objects with fields [{\"category\": \"...\", \"insurance_pct\": \"...\", \"maternity_pct\": \"...\"}]"},
        {"p3_safety_incidents", "JSON array of objects with fields [{\"category\": \"...\", \"ltifr\": \"...\", \"lost_days\": \"...\"}]"},
        {"p3_retirement_benefits", "JSON array of objects with fields [{\"benefit\": \"...\", \"emp_covered_pct\": \"...\", \"wrk_covered_pct\": \"...\"}]"},
        {"p3_parental_leave_retention", "JSON array of objects with fields [{\"category\": \"...\", \"return_rate\": \"...\", \"retention_rate\": \"...\"}]"},
        {"p3_grievance_mechanisms", "JSON array of objects with fields [{\"category\": \"...\", \"mechanism_details\": \"...\"}]"},
        {"p3_union_membership", "JSON array of objects with fields [{\"category\": \"...\", \"union_members_pct\": \"...\"}]"},
        {"p3_hs_training", "JSON array of objects with fields [{\"category\": \"...\", \"trained_no\": \"...\", \"trained_pct\": \"...\"}]"},
        {"p3_performance_reviews", "JSON array of objects with fields [{\"category\": \"...\", \"male_pct\": \"...\", \"female_pct\": \"...\"}]"},
        {"p4_stakeholder_engagement", "JSON array of objects with fields [{\"stakeholder_group\": \"...\", \"consultation_channel\": \"...\", \"purpose\": \"...\"}]"},
        {"p5_hr_training", "JSON array of objects with fields [{\"category\": \"...\", \"employees_trained_pct\": \"...\", \"workers_trained_pct\": \"...\"}]"},
        {"p5_minimum_wages", "JSON array of objects with fields [{\"category\": \"...\", \"equal_or_more_pct\": \"...\", \"less_than_pct\": \"...\"}]"},
        {"p5_remuneration", "JSON array of objects with fields [{\"category\": \"...\", \"male_median\": \"...\", \"female_median\": \"...\"}]"},
        {"p5_hr_complaints", "JSON array of objects with fields [{\"category\": \"...\", \"filed_current\": \"...\", \"pending_current\": \"...\"}]"},
        {"p5_hr_assessments", "JSON array of objects with fields [{\"aspect\": \"...\", \"plants_assessed_pct\": \"...\"}]"},
        {"p6_energy", "JSON array of objects with fields [{\"parameter\": \"...\", \"current_value\": \"...\", \"previous_value\": \"...\"}]"},
        {"p6_air_emissions", "JSON array of objects with fields [{\"parameter\": \"...\", \"unit\": \"...\", \"current_value\": \"...\", \"previous_value\": \"...\"}]"},
        {"p6_ghg_emissions", "JSON array of objects with fields [{\"parameter\": \"...\", \"current_value\": \"...\", \"previous_value\": \"...\"}]"},
        {"p6_scope123_emissions", "JSON array of objects with fields [{\"scope\": \"...\", \"current_value\": \"...\", \"previous_value\": \"...\"}]"},
        {"p6_water_withdrawal", "JSON array of objects with fields [{\"parameter\": \"...\", \"current_value\": \"...\", \"previous_value\": \"...\"}]"},
        {"p6_water_discharge", "JSON array of objects with fields [{\"destination\": \"...\", \"current_value\": \"...\", \"previous_value\": \"...\"}]"},
        {"p6_waste", "JSON array of objects with fields [{\"category\": \"...\", \"current_value\": \"...\", \"previous_value\": \"...\"}]"},
        {"p6_eia", "JSON array of objects with fields [{\"project_name\": \"...\", \"eia_notification_no\": \"...\", \"conducted_by\": \"...\"}]"},
        {"p7_trade_associations", "JSON array of objects with fields [{\"association_name\": \"...\", \"reach\": \"...\"}]"},
        {"p7_anticompetitive_cases", "JSON array of objects with fields [{\"agency\": \"...\", \"brief_case\": \"...\"}]"},
        {"p7_policy_advocacy", "JSON array of objects with fields [{\"issue_advocated\": \"...\", \"method\": \"...\"}]"},
        {"p8_sia", "JSON array of objects with fields [{\"project_name\": \"...\", \"summary\": \"...\", \"web_link\": \"...\"}]"},
        {"p8_rehabilitation", "JSON array of objects with fields [{\"project_name\": \"...\", \"pafs_affected\": \"...\"}]"},
        {"p8_csr_project_details", "JSON array of objects with fields [{\"state\": \"...\", \"district\": \"...\", \"project_name\": \"...\", \"amount_spent\": \"...\"}]"},
        {"p9_complaints", "JSON array of objects with fields [{\"category\": \"...\", \"received_current\": \"...\", \"pending_current\": \"...\"}]"},
        {"p9_product_recalls", "JSON array of objects with fields [{\"reason\": \"...\", \"number_recalled\": \"...\"}]"},
        {"brsr_core_kpis", "JSON array of objects with fields [{\"kpi_name\": \"...\", \"current_value\": \"...\", \"assurance_provider\": \"...\"}]"}
    };
    return s_schemaHints.value(key, "a JSON array of objects, one per row found in the context");
}

std::string autoRepairJson(const std::string& raw) {
    size_t firstBrace = raw.find('{');
    if (firstBrace == std::string::npos) {
        return "{}";
    }

    std::string s = raw.substr(firstBrace);
    std::vector<char> stack;
    bool inString = false;
    bool escapeNext = false;

    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (inString) {
            if (escapeNext) {
                escapeNext = false;
            } else if (c == '\\') {
                escapeNext = true;
            } else if (c == '"') {
                inString = false;
            }
        } else {
            if (c == '"') {
                inString = true;
            } else if (c == '{') {
                stack.push_back('}');
            } else if (c == '[') {
                stack.push_back(']');
            } else if (c == '}') {
                if (!stack.empty() && stack.back() == '}') {
                    stack.pop_back();
                }
            } else if (c == ']') {
                if (!stack.empty() && stack.back() == ']') {
                    stack.pop_back();
                }
            }
        }
    }

    if (inString) {
        s += '"';
    }

    // Clean up trailing comma or dangling colon before closing brackets
    size_t lastNonWs = s.find_last_not_of(" \t\r\n");
    if (lastNonWs != std::string::npos) {
        if (s[lastNonWs] == ',') {
            s.erase(lastNonWs);
        } else if (s[lastNonWs] == ':') {
            s += " null";
        }
    }

    while (!stack.empty()) {
        s += stack.back();
        stack.pop_back();
    }

    return s;
}

} // namespace

ReportExtractionService::ReportExtractionService(Database& db, LlamaEngine& llama,
                                                   VectorIndexRegistry& indexRegistry,
                                                   const BrsrSchemaCatalog& catalog,
                                                   SystemMonitor& monitor)
    : _db(db), _llama(llama), _indexRegistry(indexRegistry), _catalog(catalog), _monitor(monitor), _factStore(&db)
{
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
    std::vector<int64_t> vectorIds = _indexRegistry.getOrCreate(jobId).search(queryEmbedding, 6);

    for (int64_t vectorId : vectorIds) {
        if (vectorId >= 0 && seenVectorIds.insert(vectorId).second) {
            allIds.push_back(vectorId);
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
        if (it != chunkMap.end() && !it.value().isEmpty()) {
            if (assembledContext.length() + it.value().length() > kMaxContextChars) {
                // If we already have at least one chunk, stop to avoid prompt overflow
                if (!matchedIds.isEmpty()) {
                    break;
                }
            }
            assembledContext += it.value() + "\n\n";
            matchedIds.push_back(id);
        }
    }

    qint64 elapsedMs = retTimer.elapsed();
    return {group.id, assembledContext, matchedIds, elapsedMs};
}

std::string ReportExtractionService::buildExtractionPrompt(int jobId, const FieldGroup& group, const QString& context) const
{
    QString fieldList;
    for (const QString& key : group.dataFieldKeys) {
        fieldList += "- " + key + ": a precise numeric or short factual value\n";
    }
    for (const QString& key : group.narrativeFieldKeys) {
        fieldList += "- " + key + ": a short narrative paragraph grounded only in the context below\n";
    }
    for (const QString& key : group.loopFieldKeys) {
        fieldList += "- " + key + ": " + getLoopFieldHint(key) + "\n";
    }

    // Ground with known baseline document facts (Entity, FY, Turnover, Headcount)
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

    std::string prompt =
        "<|im_start|>system\n" + instructions.toStdString() + "<|im_end|>\n"
        "<|im_start|>user\n"
        "Context:\n" + context.toStdString() + "\n\nExtract JSON.<|im_end|>\n"
        "<|im_start|>assistant\n";
    return prompt;
}

void ReportExtractionService::generateAndPersistGroup(int jobId, const FieldGroup& group,
                                                     const GroupContext& ctx,
                                                     qint64& outPrefillMs, qint64& outDecodeMs,
                                                     int& outTokensGenerated)
{
    QElapsedTimer genTimer;
    genTimer.start();

    std::string prompt = buildExtractionPrompt(jobId, group, ctx.assembledContext);

    std::string generatedJson;
    int braceCount = 0;
    bool jsonStarted = false;
    bool inString = false;
    bool escapeNext = false;
    bool firstTokenReceived = false;
    qint64 prefillTime = 0;
    int tokenCount = 0;

    GenerationOptions options;
    options.temperature   = 0.0f;  // Deterministic greedy argmax sampling for factual ESG data
    options.maxTokens     = group.loopFieldKeys.isEmpty() ? 768 : 1536;
    options.repeatPenalty = 1.05f;

    _llama.streamCompletion(prompt, [&](const std::string& token) -> bool {
        if (_cancelRequested.load()) {
            return false;
        }
        if (!firstTokenReceived) {
            firstTokenReceived = true;
            prefillTime = genTimer.elapsed();
        }
        tokenCount++;
        generatedJson += token;

        for (size_t i = 0; i < token.size(); ++i) {
            char c = token[i];
            if (inString) {
                if (escapeNext) {
                    escapeNext = false;
                } else if (c == '\\') {
                    escapeNext = true;
                } else if (c == '"') {
                    inString = false;
                }
            } else {
                if (c == '"') {
                    inString = true;
                } else if (c == '{') {
                    braceCount++;
                    jsonStarted = true;
                } else if (c == '}') {
                    braceCount--;
                    if (jsonStarted && braceCount == 0) {
                        return false; // Complete JSON object reached
                    }
                }
            }
        }
        return true;
    });

    if (_cancelRequested.load()) {
        return;
    }

    qint64 totalGenMs = genTimer.elapsed();
    outPrefillMs = prefillTime;
    outDecodeMs = totalGenMs - prefillTime;
    outTokensGenerated = tokenCount;

    // Parse JSON & memoize facts into FactStore
    size_t firstBrace = generatedJson.find('{');
    size_t lastBrace = generatedJson.rfind('}');
    if (firstBrace != std::string::npos && lastBrace != std::string::npos && lastBrace >= firstBrace) {
        generatedJson = generatedJson.substr(firstBrace, lastBrace - firstBrace + 1);
    }

    QStringList sourceIdStrings;
    for (int64_t id : ctx.sourceChunkIds) {
        sourceIdStrings.append(QString::number(id));
    }
    QString sourceChunksStr = sourceIdStrings.join(",");

    try {
        nlohmann::json extracted;
        try {
            extracted = nlohmann::json::parse(generatedJson);
        } catch (...) {
            // Attempt auto-repair on clipped/truncated JSON output
            std::string repaired = autoRepairJson(generatedJson);
            extracted = nlohmann::json::parse(repaired);
            _log.info(QString("Successfully salvaged and parsed auto-repaired JSON for section '%1'").arg(group.id));
        }

        _db.upsertReportSection(jobId, group.id, QString::fromStdString(extracted.dump()), "DRAFT", sourceChunksStr);

        // Record metrics into FactStore and topic anchors
        int nonNullFacts = 0;
        int64_t primaryChunkId = ctx.sourceChunkIds.isEmpty() ? -1 : ctx.sourceChunkIds.first();

        for (auto it = extracted.begin(); it != extracted.end(); ++it) {
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

        // If this group had real data, anchor this topic/section for future locality hits
        if (nonNullFacts >= 2 && primaryChunkId > 0) {
            _factStore.recordTopicAnchor(jobId, group.section, primaryChunkId, 0);
        }

        // Identify which statutory data fields could not be found in document text
        QStringList missingDataKeys;
        for (const QString& key : group.dataFieldKeys) {
            std::string keyStd = key.toStdString();
            if (!extracted.contains(keyStd) || extracted[keyStd].is_null()) {
                missingDataKeys.append(key);
            } else if (extracted[keyStd].is_string()) {
                std::string s = extracted[keyStd].get<std::string>();
                if (s.empty() || s == "null" || s == "N/A") {
                    missingDataKeys.append(key);
                }
            }
        }

        if (!missingDataKeys.isEmpty()) {
            QMutexLocker locker(&_progressMutex);
            _progress.missingSections.append(MissingSection{group.id, group.label, missingDataKeys});
            _log.info(QString("[Notice] Section '%1': %2 metrics could not be located in source PDFs: %3")
                .arg(group.label).arg(missingDataKeys.size()).arg(missingDataKeys.join(", ")));
        }

        double tps = outDecodeMs > 0 ? (tokenCount / (outDecodeMs / 1000.0)) : 0.0;
        _log.info(QString("[Perf] Section '%1' | Retrieval: %2ms (%3 chunks) | Generation: %4ms (Prefill: %5ms, Decode: %6ms, %7 tok @ %8 t/s) | Facts memoized: %9")
            .arg(group.id)
            .arg(ctx.retrievalMs)
            .arg(ctx.sourceChunkIds.size())
            .arg(totalGenMs)
            .arg(outPrefillMs)
            .arg(outDecodeMs)
            .arg(tokenCount)
            .arg(tps, 0, 'f', 1)
            .arg(nonNullFacts));

    } catch (const std::exception& e) {
        _db.upsertReportSection(jobId, group.id, "{}", "FAILED", sourceChunksStr);
        {
            QMutexLocker locker(&_progressMutex);
            _progress.missingSections.append(MissingSection{group.id, group.label, group.dataFieldKeys});
        }
        _log.error(QString("[Perf] Section '%1' JSON parse failed: %2").arg(group.id).arg(e.what()));
    }
}

void ReportExtractionService::cancelJob(int jobId)
{
    if (_activeJobId.load() == jobId) {
        _log.warn(QString("Cancellation requested for extracting job %1").arg(jobId));
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

    _db.setExtractionStatus(jobId, "EXTRACTING");
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
        _db.setExtractionStatus(jobId, "FAILED");
        _log.warn(QString("Job %1 extraction cancelled before start").arg(jobId));
        return;
    }

    // 1. Initialize and load any persisted facts for this job
    _factStore.loadForJob(jobId);

    // 2. Parallel retrieval phase: one embedding + vector/locality search per group, run concurrently.
    QElapsedTimer swapTimer;
    swapTimer.start();
    if (!_llama.loadEmbdModel()) {
        _db.setExtractionStatus(jobId, "FAILED");
        _log.error(QString("Job %1: failed to load embedding model").arg(jobId));
        return;
    }
    qint64 embdModelLoadMs = swapTimer.elapsed();

    QFutureSynchronizer<GroupContext> retrievalSync;
    for (const FieldGroup& group : _catalog.groups()) {
        retrievalSync.addFuture(QtConcurrent::run([this, jobId, group]() {
            return retrieveContextForGroup(jobId, group);
        }));
    }
    retrievalSync.waitForFinished();

    QHash<QString, GroupContext> contextByGroupId;
    qint64 totalRetrievalMs = 0;
    for (const QFuture<GroupContext>& future : retrievalSync.futures()) {
        GroupContext result = future.result();
        totalRetrievalMs += result.retrievalMs;
        contextByGroupId[result.groupId] = result;
    }

    // Keep embedding model resident in background (~67MB) to avoid repeated disk reads

    if (_cancelRequested.load()) {
        _db.setExtractionStatus(jobId, "FAILED");
        _log.warn(QString("Job %1 extraction cancelled after retrieval").arg(jobId));
        return;
    }

    // 3. Serial generation phase: load generation model once
    swapTimer.restart();
    if (!_llama.loadGenModel()) {
        _db.setExtractionStatus(jobId, "FAILED");
        _log.error(QString("Job %1: failed to load generation model").arg(jobId));
        return;
    }
    qint64 genModelLoadMs = swapTimer.elapsed();

    qint64 totalPrefillMs = 0;
    qint64 totalDecodeMs = 0;
    int totalTokens = 0;
    int extractedCount = 0;
    int skippedCount = 0;

    for (const FieldGroup& group : _catalog.groups()) {
        if (_cancelRequested.load()) {
            _db.setExtractionStatus(jobId, "FAILED");
            _log.warn(QString("Job %1 extraction cancelled during section generation").arg(jobId));
            break;
        }

        {
            QMutexLocker locker(&_progressMutex);
            _progress.currentStep = extractedCount + skippedCount;
            _progress.currentGroupId = group.id;
            _progress.currentGroupLabel = group.label;
        }

        const GroupContext& ctx = contextByGroupId.value(group.id);
        if (ctx.assembledContext.isEmpty()) {
            _db.upsertReportSection(jobId, group.id, "{}", "FAILED");
            _log.info(QString("[Perf] Section '%1' skipped (zero context)").arg(group.id));
            skippedCount++;
            {
                QMutexLocker locker(&_progressMutex);
                _progress.currentStep = extractedCount + skippedCount;
            }
            continue;
        }

        qint64 prefillMs = 0;
        qint64 decodeMs = 0;
        int tokensGen = 0;
        generateAndPersistGroup(jobId, group, ctx, prefillMs, decodeMs, tokensGen);

        totalPrefillMs += prefillMs;
        totalDecodeMs += decodeMs;
        totalTokens += tokensGen;
        extractedCount++;

        {
            QMutexLocker locker(&_progressMutex);
            _progress.currentStep = extractedCount + skippedCount;
        }
    }

    // Schedule idle timeout to release 4.7GB generation model after 5 minutes of inactivity
    _llama.scheduleIdleUnload(300);

    if (_cancelRequested.load()) {
        _db.setExtractionStatus(jobId, "FAILED");
        _log.warn(QString("Job %1 extraction terminated due to cancellation").arg(jobId));
        return;
    }

    _db.setExtractionStatus(jobId, "READY_FOR_REVIEW");

    {
        QMutexLocker locker(&_progressMutex);
        _progress.currentStep = _catalog.groups().size();
        _progress.currentGroupLabel = "Extraction complete";
    }

    qint64 totalWallMs = totalJobTimer.elapsed();
    double totalWallSec = totalWallMs / 1000.0;
    double swapSec = (embdModelLoadMs + genModelLoadMs) / 1000.0;
    double retrievalSec = totalRetrievalMs / 1000.0;
    double prefillSec = totalPrefillMs / 1000.0;
    double decodeSec = totalDecodeMs / 1000.0;
    double avgTps = decodeSec > 0 ? (totalTokens / decodeSec) : 0.0;

    _log.info(QString(
        "\n======================================================================\n"
        "[Perf-Summary] Job %1 Extraction Completed in %2s:\n"
        "  ├── Model Swapping:    %3s (%4%)\n"
        "  ├── Total Retrieval:   %5s (%6%)\n"
        "  ├── Total LLM Prefill: %7s (%8%)\n"
        "  ├── Total LLM Decode:  %9s (%10%) [Avg: %11 tokens/sec, %12 tokens total]\n"
        "  └── Total Sections:    %13 (%14 extracted, %15 skipped)\n"
        "======================================================================")
        .arg(jobId)
        .arg(totalWallSec, 0, 'f', 2)
        .arg(swapSec, 0, 'f', 2)
        .arg((swapSec / totalWallSec) * 100.0, 0, 'f', 1)
        .arg(retrievalSec, 0, 'f', 2)
        .arg((retrievalSec / totalWallSec) * 100.0, 0, 'f', 1)
        .arg(prefillSec, 0, 'f', 2)
        .arg((prefillSec / totalWallSec) * 100.0, 0, 'f', 1)
        .arg(decodeSec, 0, 'f', 2)
        .arg((decodeSec / totalWallSec) * 100.0, 0, 'f', 1)
        .arg(avgTps, 0, 'f', 1)
        .arg(totalTokens)
        .arg(_catalog.groups().size())
        .arg(extractedCount)
        .arg(skippedCount));
    }
    catch (const std::exception& e)
    {
        _llama.unloadEmbdModel();
        _llama.unloadGenModel();
        _db.setExtractionStatus(jobId, "FAILED");
        _log.error(QString("Job %1 extraction failed with unhandled exception: %2").arg(jobId).arg(e.what()));
    }
    catch (...)
    {
        _llama.unloadEmbdModel();
        _llama.unloadGenModel();
        _db.setExtractionStatus(jobId, "FAILED");
        _log.error(QString("Job %1 extraction failed with unknown exception").arg(jobId));
    }
}

ReportExtractionService::ProgressInfo ReportExtractionService::currentProgress() const
{
    QMutexLocker locker(&_progressMutex);
    return _progress;
}

