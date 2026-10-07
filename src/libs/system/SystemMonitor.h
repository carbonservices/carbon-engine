#pragma once

#include "logging/Logger.h"

// Qt
#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QString>

// C++
#include <atomic>

// ──────────────────────────────────────────────────────────────
// Engine phase enum — call setEnginePhase() at phase boundaries
// in LlamaEngine and Database so the UI reflects live activity.
// ──────────────────────────────────────────────────────────────
enum class EnginePhase {
    Idle,
    LoadingModel,
    Embedding,
    Inferencing,
    DbReading,
    DbWriting
};

// ──────────────────────────────────────────────────────────────
// SystemMonitor — singleton-like, one instance lives in main().
// Samples CPU/memory periodically; GPU presence detected once
// at startup; engine phase is set externally at call-sites.
// ──────────────────────────────────────────────────────────────
class SystemMonitor : public QObject
{
    Q_OBJECT
public:
    explicit SystemMonitor(QObject* parent = nullptr);

    // Called by LlamaEngine / Database at phase transitions.
    void setEnginePhase(EnginePhase phase);
    EnginePhase enginePhase() const;

    // Called by LlamaEngine after each streamCompletion.
    void setTokensPerSec(double tps);

    // ── Snapshot accessors (safe to call from any thread) ─────
    double  cpuPctProcess()   const;
    double  cpuPctSystem()    const;
    quint64 memUsedMbProcess() const;
    quint64 memUsedMbSystem()  const;
    quint64 memTotalMbSystem() const;
    bool    gpuPresent()      const;
    double  tokensPerSec()    const;

    // Human-readable engine phase string for JSON.
    static QString phaseString(EnginePhase phase);

private slots:
    void sample();

private:
    void detectGpu();

    QTimer              _timer;
    mutable QMutex      _mutex;

    // Sampled periodically
    double  _cpuPctProcess   = 0.0;
    double  _cpuPctSystem    = 0.0;
    quint64 _memUsedMbProcess = 0;
    quint64 _memUsedMbSystem  = 0;
    quint64 _memTotalMbSystem = 0;

    // Set externally
    std::atomic<EnginePhase> _phase{EnginePhase::Idle};
    std::atomic<double>      _tokensPerSec{0.0};

    // Detected once
    bool _gpuPresent = false;

    // Windows CPU-time bookkeeping (FILETIME stored as ULONGLONG)
    quint64 _prevSysIdle   = 0;
    quint64 _prevSysKernel = 0;
    quint64 _prevSysUser   = 0;
    quint64 _prevProcKernel = 0;
    quint64 _prevProcUser   = 0;

    Logger _log{"SystemMonitor"};

    static constexpr int kSampleIntervalMs = 1000;
};
