#ifndef SYSTEM_STATUS_POLLER_H
#define SYSTEM_STATUS_POLLER_H

#include "logging/Logger.h"

// Qt
#include <QObject>
#include <QQmlEngine>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QString>
#include <QUrl>

class SystemStatusPoller : public QObject
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

    Q_INVOKABLE void startPolling();
    Q_INVOKABLE void stopPolling();

signals:
    void statusUpdated();

private slots:
    void poll();

private:
    static constexpr int PollIntervalMs = 1000;
    static inline const QUrl Endpoint{"http://localhost:8080/api/system"};

    QNetworkAccessManager _network;
    QTimer _timer;

    QString _jobQueue = "0";
    QString _cpuLoad = "0% (0% sys)";
    QString _memoryUsage = "0 MB (0 MB sys)";
    QString _gpuLoad = "N/A";
    QString _gpuOffload = "N/A";
    QString _vramUsage = "N/A";
    QString _engineStatus = "Idle";
    QString _databaseStatus = "Idle";
    QString _tokenRate = "0 t/s";

    Logger _log{"QmlClient.SystemStatusPoller"};
};

#endif // SYSTEM_STATUS_POLLER_H
