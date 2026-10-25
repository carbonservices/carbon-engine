#ifndef SYSTEM_STATUS_POLLER_H
#define SYSTEM_STATUS_POLLER_H

#include "ApiClientBase.h"

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
    Q_PROPERTY(bool cloudEnabled READ cloudEnabled CONSTANT)
    Q_PROPERTY(QString activeEngineMode READ activeEngineMode CONSTANT)
    Q_PROPERTY(QString activeModel READ activeModel NOTIFY statusUpdated)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY statusUpdated)

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
    bool cloudEnabled() const { return false; }
    QString activeEngineMode() const { return QStringLiteral("OFFLINE"); }
    QString activeModel() const { return _activeModel; }
    bool isConnected() const { return _isConnected; }

    Q_INVOKABLE void startPolling();
    Q_INVOKABLE void stopPolling();

signals:
    void statusUpdated();

private slots:
    void poll();

private:
    static constexpr int PollIntervalMs = 1000;

    QTimer _timer;

    bool _isConnected = false;
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

    Logger _log{"QmlClient.SystemStatusPoller"};
};

#endif // SYSTEM_STATUS_POLLER_H
