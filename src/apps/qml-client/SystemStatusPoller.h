#ifndef SYSTEM_STATUS_POLLER_H
#define SYSTEM_STATUS_POLLER_H

#include "ApiClientBase.h"
#include "constants/CommonConstants.h"

// Qt
#include <QQmlEngine>
#include <QTimer>
#include <QString>

class SystemStatusPoller : public ApiClientBase
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString jobQueue READ jobQueue NOTIFY statusUpdated)
    Q_PROPERTY(QString cpuLoad READ cpuLoad NOTIFY statusUpdated)
    Q_PROPERTY(QString memoryUsage READ memoryUsage NOTIFY statusUpdated)
    Q_PROPERTY(QString gpuLoad READ gpuLoad NOTIFY statusUpdated)
    Q_PROPERTY(QString gpuOffload READ gpuOffload NOTIFY statusUpdated)
    Q_PROPERTY(QString vramUsage READ vramUsage NOTIFY statusUpdated)
    Q_PROPERTY(QString engineStatus READ engineStatus NOTIFY statusUpdated)
    Q_PROPERTY(QString databaseStatus READ databaseStatus NOTIFY statusUpdated)
    Q_PROPERTY(QString tokenRate READ tokenRate NOTIFY statusUpdated)
    Q_PROPERTY(QString activeModel READ activeModel NOTIFY statusUpdated)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY statusUpdated)
    Q_PROPERTY(QString workloadTitle READ workloadTitle NOTIFY statusUpdated)
    Q_PROPERTY(QString workloadTag READ workloadTag NOTIFY statusUpdated)
    Q_PROPERTY(bool forceCpu READ forceCpu NOTIFY settingsUpdated)
    Q_PROPERTY(bool forceGpu READ forceGpu NOTIFY settingsUpdated)
    Q_PROPERTY(bool systemSpecsPrecedence READ systemSpecsPrecedence NOTIFY settingsUpdated)
    Q_PROPERTY(QString activeInferenceMode READ activeInferenceMode NOTIFY statusUpdated)
    Q_PROPERTY(QString gpuName READ gpuName NOTIFY statusUpdated)
    Q_PROPERTY(bool gpuPresent READ gpuPresent NOTIFY statusUpdated)

public:
    explicit SystemStatusPoller(QObject *parent = nullptr);

    QString jobQueue() const { return _jobQueue; }
    QString cpuLoad() const { return _cpuLoad; }
    QString memoryUsage() const { return _memoryUsage; }
    QString gpuLoad() const { return _gpuLoad; }
    QString gpuOffload() const { return _gpuOffload; }
    QString vramUsage() const { return _vramUsage; }
    QString engineStatus() const { return _engineStatus; }
    QString databaseStatus() const { return _databaseStatus; }
    QString tokenRate() const { return _tokenRate; }
    QString activeModel() const { return _activeModel; }
    bool isConnected() const { return _isConnected; }
    QString workloadTitle() const { return _workloadTitle; }
    QString workloadTag() const { return _workloadTag; }

    bool forceCpu() const { return _forceCpu; }
    bool forceGpu() const { return _forceGpu; }
    bool systemSpecsPrecedence() const { return _systemSpecsPrecedence; }
    QString activeInferenceMode() const { return _activeInferenceMode; }
    QString gpuName() const { return _gpuName; }
    bool gpuPresent() const { return _gpuPresent; }

    Q_INVOKABLE void setForceCpuMode(bool enabled);
    Q_INVOKABLE void setForceGpuMode(bool enabled);
    Q_INVOKABLE void resetToSystemSpecs();
    Q_INVOKABLE void refreshSettings();

    Q_INVOKABLE void startPolling();
    Q_INVOKABLE void stopPolling();

signals:
    void statusUpdated();
    void settingsUpdated();

private slots:
    void poll();
    void onPollFinished();
    void onSettingsPostFinished();
    void onSettingsGetFinished();

private:
    static constexpr int PollIntervalMs = 1000;

    QTimer _timer;

    bool _isConnected = false;
    QString _workloadTitle = QString::fromUtf8(AppInfo::WorkloadTitle);
    QString _workloadTag = QString::fromUtf8(AppInfo::WorkloadTag);
    QString _jobQueue = "—";
    QString _cpuLoad = "—";
    QString _memoryUsage = "—";
    QString _gpuLoad = "—";
    QString _gpuOffload = "—";
    QString _vramUsage = "—";
    QString _engineStatus = "Connecting...";
    QString _databaseStatus = "Offline";
    QString _tokenRate = "—";
    QString _activeModel{Models::LocalGemmaName};

    bool _forceCpu = false;
    bool _forceGpu = false;
    bool _systemSpecsPrecedence = true;
    QString _activeInferenceMode = "Auto (System Specs)";
    QString _gpuName = "";
    bool _gpuPresent = false;

    Logger _log{"QmlClient.SystemStatusPoller"};
};

#endif // SYSTEM_STATUS_POLLER_H
