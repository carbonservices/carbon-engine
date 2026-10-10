#ifndef PIPELINE_STATUS_POLLER_H
#define PIPELINE_STATUS_POLLER_H

#include "logging/Logger.h"

// Qt
#include <QObject>
#include <QQmlEngine>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QJsonArray>

class PipelineStatusPoller : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit PipelineStatusPoller(QObject *parent = Q_NULLPTR);
    Q_INVOKABLE void startPolling(int jobId);

signals:
    void completed();
    void failed(const QString &reason);
    void statusUpdated(const QString &status, const QJsonArray &documents);

private:
    void poll();

    static constexpr int PollIntervalMs = 1000;

    QNetworkAccessManager _network;
    QTimer _timer;
    int _jobId = -1;
    Logger _log{"QmlClient.PipelineStatusPoller"};
};

#endif // PIPELINE_STATUS_POLLER_H