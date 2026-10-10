#include "SystemMonitor.h"

// Windows APIs for CPU/memory/GPU
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <dxgi.h>

// Qt
#include <QMutexLocker>
#include <QThread>
#include <algorithm>

// ─────────────────────────────────────────────────────────────
// Helper: convert a FILETIME to a 64-bit integer
// ─────────────────────────────────────────────────────────────
static quint64 fileTimeToUll(const FILETIME& ft)
{
    ULARGE_INTEGER uli;
    uli.LowPart  = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart;
}

// ─────────────────────────────────────────────────────────────
SystemMonitor::SystemMonitor(QObject* parent)
    : QObject(parent)
{
    detectGpu();

    // Seed baseline process times so first delta isn't 0/0.
    FILETIME dum1, dum2, k, u;
    if (GetProcessTimes(GetCurrentProcess(), &dum1, &dum2, &k, &u)) {
        _prevProcKernel = fileTimeToUll(k);
        _prevProcUser   = fileTimeToUll(u);
    }
    FILETIME idle, kernel, user;
    if (GetSystemTimes(&idle, &kernel, &user)) {
        _prevSysIdle   = fileTimeToUll(idle);
        _prevSysKernel = fileTimeToUll(kernel);
        _prevSysUser   = fileTimeToUll(user);
    }

    connect(&_timer, &QTimer::timeout, this, &SystemMonitor::sample);
    _timer.start(kSampleIntervalMs);
}

// ─────────────────────────────────────────────────────────────
void SystemMonitor::detectGpu()
{
    // Enumerate DXGI adapters; flag true if at least one
    // non-Microsoft Basic Render Driver is present.
    IDXGIFactory* factory = nullptr;
    if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory),
                                  reinterpret_cast<void**>(&factory)))) {
        _gpuPresent = false;
        return;
    }

    IDXGIAdapter* adapter = nullptr;
    for (UINT i = 0; factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC desc;
        if (SUCCEEDED(adapter->GetDesc(&desc))) {
            // VendorId 0x1414 is the Microsoft Basic Render Driver — skip it.
            if (desc.VendorId != 0x1414) {
                _gpuPresent = true;
                _vramTotalMb = static_cast<quint64>(desc.DedicatedVideoMemory / (1024 * 1024));
                _log.info(QString("GPU detected: %1 (VRAM: %2 MB)")
                              .arg(QString::fromWCharArray(desc.Description))
                              .arg(_vramTotalMb));
                adapter->Release();
                break;
            }
        }
        adapter->Release();
    }
    factory->Release();
}

// ─────────────────────────────────────────────────────────────
void SystemMonitor::sample()
{
    QMutexLocker lock(&_mutex);

    // ── Process CPU ───────────────────────────────────────────
    {
        FILETIME c, e, k, u;
        if (GetProcessTimes(GetCurrentProcess(), &c, &e, &k, &u)) {
            quint64 curK = fileTimeToUll(k);
            quint64 curU = fileTimeToUll(u);
            quint64 dK   = curK - _prevProcKernel;
            quint64 dU   = curU - _prevProcUser;
            _prevProcKernel = curK;
            _prevProcUser   = curU;
            // Each 100-ns tick == 1 unit; interval is kSampleIntervalMs ms.
            // Normalize across all logical cores so process CPU usage is 0-100%.
            int numCores = std::max(1, QThread::idealThreadCount());
            quint64 intervalTicks = static_cast<quint64>(kSampleIntervalMs) * 10000ULL * static_cast<quint64>(numCores);
            _cpuPctProcess = (intervalTicks > 0)
                ? (static_cast<double>(dK + dU) / static_cast<double>(intervalTicks)) * 100.0
                : 0.0;
        }
    }

    // ── System CPU ────────────────────────────────────────────
    {
        FILETIME idle, kernel, user;
        if (GetSystemTimes(&idle, &kernel, &user)) {
            quint64 curIdle   = fileTimeToUll(idle);
            quint64 curKernel = fileTimeToUll(kernel);
            quint64 curUser   = fileTimeToUll(user);
            quint64 dIdle   = curIdle   - _prevSysIdle;
            quint64 dKernel = curKernel - _prevSysKernel;
            quint64 dUser   = curUser   - _prevSysUser;
            _prevSysIdle   = curIdle;
            _prevSysKernel = curKernel;
            _prevSysUser   = curUser;
            // Kernel includes idle; total = (kernel - idle) + user
            quint64 total = (dKernel - dIdle) + dUser;
            quint64 denom = (dKernel - dIdle) + dUser + dIdle;
            _cpuPctSystem = (denom > 0)
                ? (static_cast<double>(total) / static_cast<double>(denom)) * 100.0
                : 0.0;
        }
    }

    // ── Process memory ────────────────────────────────────────
    {
        PROCESS_MEMORY_COUNTERS pmc{};
        pmc.cb = sizeof(pmc);
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            _memUsedMbProcess = pmc.WorkingSetSize / (1024ULL * 1024ULL);
        }
    }

    // ── System memory ─────────────────────────────────────────
    {
        MEMORYSTATUSEX ms{};
        ms.dwLength = sizeof(ms);
        if (GlobalMemoryStatusEx(&ms)) {
            _memTotalMbSystem = ms.ullTotalPhys / (1024ULL * 1024ULL);
            _memUsedMbSystem  = (_memTotalMbSystem > ms.ullAvailPhys / (1024ULL * 1024ULL))
                ? _memTotalMbSystem - ms.ullAvailPhys / (1024ULL * 1024ULL)
                : 0;
        }
    }
}

// ─────────────────────────────────────────────────────────────
void SystemMonitor::setEnginePhase(EnginePhase phase)
{
    _phase.store(phase);
}

EnginePhase SystemMonitor::enginePhase() const
{
    return _phase.load();
}

void SystemMonitor::setTokensPerSec(double tps)
{
    _tokensPerSec.store(tps);
}

// ─────────────────────────────────────────────────────────────
double SystemMonitor::cpuPctProcess() const
{
    QMutexLocker lock(&_mutex);
    return _cpuPctProcess;
}

double SystemMonitor::cpuPctSystem() const
{
    QMutexLocker lock(&_mutex);
    return _cpuPctSystem;
}

quint64 SystemMonitor::memUsedMbProcess() const
{
    QMutexLocker lock(&_mutex);
    return _memUsedMbProcess;
}

quint64 SystemMonitor::memUsedMbSystem() const
{
    QMutexLocker lock(&_mutex);
    return _memUsedMbSystem;
}

quint64 SystemMonitor::memTotalMbSystem() const
{
    QMutexLocker lock(&_mutex);
    return _memTotalMbSystem;
}

bool SystemMonitor::gpuPresent() const
{
    return _gpuPresent;
}

quint64 SystemMonitor::vramTotalMb() const
{
    return _vramTotalMb;
}

double SystemMonitor::tokensPerSec() const
{
    return _tokensPerSec.load();
}

// ─────────────────────────────────────────────────────────────
QString SystemMonitor::phaseString(EnginePhase phase)
{
    switch (phase) {
    case EnginePhase::Idle:         return QStringLiteral("Idle");
    case EnginePhase::LoadingModel: return QStringLiteral("Loading model");
    case EnginePhase::Embedding:    return QStringLiteral("Embedding");
    case EnginePhase::Inferencing:  return QStringLiteral("Inferencing");
    case EnginePhase::DbReading:    return QStringLiteral("DB Reading");
    case EnginePhase::DbWriting:    return QStringLiteral("DB Writing");
    }
    return QStringLiteral("Unknown");
}
