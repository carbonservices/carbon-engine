#pragma once
#include <atomic>
#include <QElapsedTimer>
#include <QString>
#include <QVector>
#include <QMutex>
#include "BrsrSchema.h"
#include "FactStore.h"
#include "db/Database.h"
#include "inference/LlamaEngine.h"
#include "logging/Logger.h"
#include "system/SystemMonitor.h"
#include "vector/VectorIndexRegistry.h"

// Phase 5: High-Precision Report Extraction Service
// Features:
// 1. Consolidated SEBI statutory sections (~14 balanced groups).
// 2. FactStore memoization & topic-locality provenance.
// 3. FAISS k=6 + neighbor window retrieval.
// 4. Granular per-section and aggregated performance telemetry in logs.
class ReportExtractionService {
public:
    struct MissingSection {
        QString groupId;
        QString groupLabel;
        QStringList missingKeys;
    };

    struct ProgressInfo {
        int currentStep{0};
        int totalSteps{0};
        QString currentGroupId;
        QString currentGroupLabel;
        QVector<MissingSection> missingSections;
    };

    ReportExtractionService(Database& db, LlamaEngine& llama, VectorIndexRegistry& indexRegistry,
                             const BrsrSchemaCatalog& catalog, SystemMonitor& monitor);

    void extractJob(int jobId);

    bool isExtracting() const { return _isExtracting.load(); }
    int activeJobId() const { return _activeJobId.load(); }
    void cancelJob(int jobId);

    FactStore& factStore() { return _factStore; }
    ProgressInfo currentProgress() const;

private:
    struct GroupContext {
        QString groupId;
        QString assembledContext;
        QVector<int64_t> sourceChunkIds;
        qint64 retrievalMs{0};
    };

    GroupContext retrieveContextForGroup(int jobId, const FieldGroup& group);
    std::string buildExtractionPrompt(int jobId, const FieldGroup& group, const QString& context) const;
    void generateAndPersistGroup(int jobId, const FieldGroup& group, const GroupContext& ctx,
                                 qint64& outPrefillMs, qint64& outDecodeMs, int& outTokensGenerated);

    Database& _db;
    LlamaEngine& _llama;
    VectorIndexRegistry& _indexRegistry;
    const BrsrSchemaCatalog& _catalog;
    SystemMonitor& _monitor;
    FactStore _factStore;
    std::atomic<bool> _isExtracting{false};
    std::atomic<int> _activeJobId{-1};
    std::atomic<bool> _cancelRequested{false};
    mutable QMutex _progressMutex;
    ProgressInfo _progress;
    Logger _log{"ReportExtractionService"};
};
