#ifndef EXTRACTION_STATUS_POLLER_H
#define EXTRACTION_STATUS_POLLER_H

#include "ApiClientBase.h"

// Qt
#include <QQmlEngine>
#include <QTimer>
#include <QSet>
#include <QStringList>

class ExtractionStatusPoller : public ApiClientBase
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ExtractionStatusPoller(QObject *parent = nullptr);
    Q_INVOKABLE void startPolling(int jobId);
    Q_INVOKABLE void stopPolling();

signals:
    void completed();
    void cancelled();
    void failed(const QString &reason);
    void progressUpdated(int currentStep, int totalSteps, const QString &groupLabel, double progressPct);
    void missingMetricsReceived(const QString &groupLabel, const QStringList &missingKeys);

private slots:
    void onPollFinished();

private:
    void poll();

    static constexpr int PollIntervalMs = 1000;

    QTimer _timer;
    int _jobId = -1;
    QSet<QString> _reportedMissingSections;
    int _lastStep{-1};
    QString _lastGroup;
    Logger _log{"QmlClient.ExtractionStatusPoller"};
};

#endif // EXTRACTION_STATUS_POLLER_H
