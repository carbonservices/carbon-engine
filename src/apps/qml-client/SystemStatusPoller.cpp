#include "SystemStatusPoller.h"

// Qt
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

SystemStatusPoller::SystemStatusPoller(QObject *parent)
    : QObject(parent)
{
    connect(&_timer, &QTimer::timeout, this, &SystemStatusPoller::poll);
    startPolling();
}

void SystemStatusPoller::startPolling()
{
    if (!_timer.isActive()) {
        _timer.start(PollIntervalMs);
    }
    poll();
}

void SystemStatusPoller::stopPolling()
{
    _timer.stop();
}

void SystemStatusPoller::poll()
{
    QNetworkReply *reply = _network.get(QNetworkRequest(Endpoint));
    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            return;
        }

        auto doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            return;
        }

        QJsonObject obj = doc.object();

        // 1. Job queue
        int queueDepth = obj.value("job_queue_depth").toInt(0);
        _jobQueue = QString::number(queueDepth);

        // 2. CPU
        double cpuProc = obj.value("cpu_pct_process").toDouble(0.0);
        double cpuSys  = obj.value("cpu_pct_system").toDouble(0.0);
        _cpuLoad = QString("%1% (%2% sys)").arg(QString::number(cpuProc, 'f', 1), QString::number(cpuSys, 'f', 1));

        // 3. Memory
        qint64 memProc = obj.value("memory_used_mb_process").toInteger();
        qint64 memSys  = obj.value("memory_used_mb_system").toInteger();
        qint64 memTotal = obj.value("memory_total_mb_system").toInteger();
        if (memTotal > 0) {
            _memoryUsage = QString("%1 MB (%2/%3 MB)").arg(QString::number(memProc), QString::number(memSys), QString::number(memTotal));
        } else {
            _memoryUsage = QString("%1 MB (%2 MB sys)").arg(QString::number(memProc), QString::number(memSys));
        }

        // 4. GPU & Offload metrics
        bool gpuPresent = obj.value("gpu_present").toBool(false);
        QJsonValue gpuPctVal = obj.value("gpu_pct");
        if (!gpuPresent || gpuPctVal.isNull()) {
            _gpuLoad = "N/A";
        } else {
            _gpuLoad = QString("%1%").arg(QString::number(gpuPctVal.toDouble(0.0), 'f', 1));
        }

        qint64 vramMb = obj.value("vram_total_mb").toInteger();
        _vramUsage = (gpuPresent && vramMb > 0) ? QString("%1 MB").arg(vramMb) : "N/A";
        _gpuOffload = obj.value("gpu_offload_mode").toString(gpuPresent ? "Full GPU" : "CPU");

        // 5. Engine status
        _engineStatus = obj.value("engine_status").toString("Idle");

        // 6. Database status
        _databaseStatus = obj.value("db_status").toString("Idle");

        // 7. Token rate
        double tps = obj.value("tokens_per_sec").toDouble(0.0);
        _tokenRate = QString("%1 t/s").arg(QString::number(tps, 'f', 1));

        emit statusUpdated();
    });
}
