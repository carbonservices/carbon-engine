#ifndef PIPELINE_STATUS_POLLER_H
#define PIPELINE_STATUS_POLLER_H

#include "ApiClientBase.h"

// Qt
#include <QQmlEngine>
#include <QTimer>
#include <QJsonArray>
#include <QPointer>

class QNetworkReply;

class PipelineStatusPoller : public ApiClientBase
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit PipelineStatusPoller(QObject *parent = nullptr);
    Q_INVOKABLE void startPolling(int jobId);
    Q_INVOKABLE void stopPolling();

signals:
    void completed();
    void cancelled();
    void failed(const QString &reason);
    void statusUpdated(const QString &status, const QJsonArray &documents);

private slots:
    void onPollFinished();

private:
    void poll();

    static constexpr int PollIntervalMs = 1000;

    QTimer _timer;
    int _jobId = CoreConstants::InvalidJobId;
    QPointer<QNetworkReply> _activeReply;
    Logger _log{"QmlClient.PipelineStatusPoller"};
};

#endif // PIPELINE_STATUS_POLLER_H
