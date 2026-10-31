#include "SystemStatusPoller.h"

// Qt
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <cmath>

SystemStatusPoller::SystemStatusPoller(QObject *parent)
    : ApiClientBase(parent)
{
    connect(&_timer, &QTimer::timeout, this, &SystemStatusPoller::poll);
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
    if (_activeReply) {
        _activeReply->abort();
        _activeReply->deleteLater();
        _activeReply = nullptr;
    }
}

void SystemStatusPoller::poll()
{
    if (_activeReply) {
        return; // Discard polling tick if previous status query is still in-flight
    }
    QNetworkReply *reply = network().get(QNetworkRequest(endpoint(ApiRoutes::System)));
    _activeReply = reply;
    connect(reply, &QNetworkReply::finished, this, &SystemStatusPoller::onPollFinished);
}

void SystemStatusPoller::onPollFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    if (reply == _activeReply) {
        _activeReply = nullptr;
    }
    reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (_isConnected) {
                _log.warn(QString("Lost connection to carbon-engine daemon: %1").arg(reply->errorString()));
            }
            _isConnected = false;
            _engineStatus = "Disconnected";
            _databaseStatus = "Offline";
            _jobQueue = "—";
            _cpuLoad = "—";
            _memoryUsage = "—";
            _gpuLoad = "—";
            _gpuOffload = "—";
            _vramUsage = "—";
            _tokenRate = "—";
            _activeModel = "Unavailable";
            emit statusUpdated();
            return;
        }

        auto doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            if (_isConnected) {
                _log.warn("Invalid JSON object received from /api/system");
            }
            _isConnected = false;
            _engineStatus = "Invalid Response";
            emit statusUpdated();
            return;
        }

        bool wasConnected = _isConnected;
        _isConnected = true;
        if (!wasConnected) {
            _log.info("Established connection to carbon-engine daemon.");
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
            double sysUsedGb = static_cast<double>(memSys) / 1024.0;
            double sysTotalGb = static_cast<double>(memTotal) / 1024.0;
            _memoryUsage = QString("%1 MB (%2 / %3 GB sys)").arg(
                QString::number(memProc),
                QString::number(sysUsedGb, 'f', 1),
                QString::number(sysTotalGb, 'f', 1));
        } else if (memSys > 0) {
            _memoryUsage = QString("%1 MB (%2 MB sys)").arg(QString::number(memProc), QString::number(memSys));
        } else {
            _memoryUsage = QString("%1 MB").arg(QString::number(memProc));
        }

        // 4. GPU & Offload metrics
        bool gpuPresent = obj.value("gpu_present").toBool(false);
        QJsonValue gpuPctVal = obj.value("gpu_pct");
        if (!gpuPresent || gpuPctVal.isNull()) {
            _gpuLoad = "N/A";
        } else {
            _gpuLoad = QString("%1%").arg(QString::number(gpuPctVal.toDouble(0.0), 'f', 1));
        }

        qint64 vramUsedMb = obj.value("vram_used_mb").toInteger();
        qint64 vramTotalMb = obj.value("vram_total_mb").toInteger();
        if (gpuPresent && vramTotalMb > 0) {
            if (vramUsedMb > 0) {
                if (vramTotalMb >= 1024) {
                    double usedGb = static_cast<double>(vramUsedMb) / 1024.0;
                    double totGb = static_cast<double>(vramTotalMb) / 1024.0;
                    int pct = static_cast<int>(std::round((static_cast<double>(vramUsedMb) / static_cast<double>(vramTotalMb)) * 100.0));
                    _vramUsage = QString("%1 / %2 GB (%3%)")
                                     .arg(QString::number(usedGb, 'f', 1))
                                     .arg(QString::number(totGb, 'f', 1))
                                     .arg(pct);
                } else {
                    int pct = static_cast<int>(std::round((static_cast<double>(vramUsedMb) / static_cast<double>(vramTotalMb)) * 100.0));
                    _vramUsage = QString("%1 / %2 MB (%3%)").arg(vramUsedMb).arg(vramTotalMb).arg(pct);
                }
            } else {
                if (vramTotalMb >= 1024) {
                    double totGb = static_cast<double>(vramTotalMb) / 1024.0;
                    _vramUsage = QString("%1 GB").arg(QString::number(totGb, 'f', 1));
                } else {
                    _vramUsage = QString("%1 MB").arg(vramTotalMb);
                }
            }
        } else {
            _vramUsage = "N/A";
        }

        _gpuOffload = obj.value("gpu_offload_mode").toString(gpuPresent ? "Full GPU" : "CPU");

        // 5. Engine status
        _engineStatus = obj.value("engine_status").toString("Idle");

        // 6. Database status
        _databaseStatus = obj.value("db_status").toString("Idle");

        // 7. Token rate
        double tps = obj.value("tokens_per_sec").toDouble(0.0);
        if (tps > 0.0) {
            _tokenRate = QString("%1 t/s").arg(QString::number(tps, 'f', 1));
        } else if (_engineStatus == "Inferencing" || _engineStatus == "Processing") {
            _tokenRate = "Calculating...";
        } else {
            _tokenRate = "—";
        }

        _activeModel = obj.value("active_model").toString(Models::LocalGemmaName);
        _workloadTitle = obj.value("workload_title").toString(_workloadTitle);
        _workloadTag = obj.value("workload_tag").toString(_workloadTag);

        _activeInferenceMode = obj.value("inference_mode").toString(_gpuOffload);
        _forceCpu = obj.value("force_cpu").toBool(_forceCpu);
        _forceGpu = obj.value("force_gpu").toBool(_forceGpu);
        _systemSpecsPrecedence = obj.value("system_specs_precedence").toBool(!_forceCpu && !_forceGpu);
        _gpuName = obj.value("gpu_device_name").toString(obj.value("gpu_name").toString(_gpuName));
        _gpuPresent = gpuPresent;

        emit statusUpdated();
}

void SystemStatusPoller::setForceCpuMode(bool enabled)
{
    QJsonObject req;
    req["force_cpu"] = enabled;
    if (enabled) {
        req["force_gpu"] = false;
    }
    QByteArray body = QJsonDocument(req).toJson(QJsonDocument::Compact);

    QNetworkRequest request(endpoint(ApiRoutes::Settings));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = network().post(request, body);
    connect(reply, &QNetworkReply::finished, this, &SystemStatusPoller::onSettingsPostFinished);
}

void SystemStatusPoller::setForceGpuMode(bool enabled)
{
    QJsonObject req;
    req["force_gpu"] = enabled;
    if (enabled) {
        req["force_cpu"] = false;
    }
    QByteArray body = QJsonDocument(req).toJson(QJsonDocument::Compact);

    QNetworkRequest request(endpoint(ApiRoutes::Settings));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = network().post(request, body);
    connect(reply, &QNetworkReply::finished, this, &SystemStatusPoller::onSettingsPostFinished);
}

void SystemStatusPoller::resetToSystemSpecs()
{
    QJsonObject req;
    req["force_cpu"] = false;
    req["force_gpu"] = false;
    QByteArray body = QJsonDocument(req).toJson(QJsonDocument::Compact);

    QNetworkRequest request(endpoint(ApiRoutes::Settings));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = network().post(request, body);
    connect(reply, &QNetworkReply::finished, this, &SystemStatusPoller::onSettingsPostFinished);
}

void SystemStatusPoller::refreshSettings()
{
    QNetworkReply *reply = network().get(QNetworkRequest(endpoint(ApiRoutes::Settings)));
    connect(reply, &QNetworkReply::finished, this, &SystemStatusPoller::onSettingsGetFinished);
}

void SystemStatusPoller::onSettingsPostFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() == QNetworkReply::NoError) {
        auto doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            _forceCpu = obj.value("force_cpu").toBool(false);
            _forceGpu = obj.value("force_gpu").toBool(false);
            _systemSpecsPrecedence = obj.value("system_specs_precedence").toBool(true);
            _activeInferenceMode = obj.value("active_mode").toString(_activeInferenceMode);
            _gpuName = obj.value("gpu_name").toString(_gpuName);
            _gpuPresent = obj.value("gpu_present").toBool(_gpuPresent);
            emit settingsUpdated();
            emit statusUpdated();
        }
    } else {
        _log.warn(QString("Failed to update settings: %1").arg(reply->errorString()));
    }
}

void SystemStatusPoller::onSettingsGetFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() == QNetworkReply::NoError) {
        auto doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            _forceCpu = obj.value("force_cpu").toBool(false);
            _forceGpu = obj.value("force_gpu").toBool(false);
            _systemSpecsPrecedence = obj.value("system_specs_precedence").toBool(true);
            _activeInferenceMode = obj.value("active_mode").toString(_activeInferenceMode);
            _gpuName = obj.value("gpu_name").toString(_gpuName);
            _gpuPresent = obj.value("gpu_present").toBool(_gpuPresent);
            emit settingsUpdated();
            emit statusUpdated();
        }
    } else {
        _log.warn(QString("Failed to retrieve settings: %1").arg(reply->errorString()));
    }
}
