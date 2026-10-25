#pragma once

#include <atomic>
#include <QElapsedTimer>
#include <QString>
#include <QVector>
#include <QMutex>

#include "constants/CommonConstants.h"
#include "report/BRSRSchema.h"
#include "report/FactStore.h"
#include "db/Database.h"
#include "inference/LlamaEngine.h"
#include "logging/Logger.h"
#include "system/SystemMonitor.h"
#include "vector/VectorIndexRegistry.h"

#include <QObject>

class ReportExtractionService : public QObject {
    Q_OBJECT

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
        QString engineMode{EngineMode::Offline};
        QString activeModel;
        qint64 lastLatencyMs{0};
    };

    ReportExtractionService(Database& db, LlamaEngine& llama,
                            VectorIndexRegistry& indexRegistry,
                            const BRSRSchemaCatalog& catalog, SystemMonitor& monitor,
                            QObject* parent = nullptr);

    bool isExtracting() const { return _isExtracting.load(); }
    int activeJobId() const { return _activeJobId.load(); }

public slots:
    void extractJob(int jobId);
    void startAsyncExtraction(int jobId);
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
    std::string buildPrompt(int jobId, const FieldGroup& group, const QString& context) const;
    QString buildInstructions(int jobId, const FieldGroup& group) const;

    void generateAndPersistGroup(int jobId, const FieldGroup& group, const GroupContext& ctx,
                                 qint64& outPrefillMs, qint64& outDecodeMs, int& outTokensGenerated);

    Database& _db;
    LlamaEngine& _llama;
    VectorIndexRegistry& _indexRegistry;
    const BRSRSchemaCatalog& _catalog;
    SystemMonitor& _monitor;
    FactStore _factStore;

    std::atomic<bool> _isExtracting{false};
    std::atomic<int> _activeJobId{-1};
    std::atomic<bool> _cancelRequested{false};

    mutable QMutex _progressMutex;
    ProgressInfo _progress;
    Logger _log{"ReportExtractionService"};
};

