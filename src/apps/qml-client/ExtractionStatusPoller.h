#ifndef EXTRACTION_STATUS_POLLER_H
#define EXTRACTION_STATUS_POLLER_H

#include "logging/Logger.h"

// Qt
#include <QObject>
#include <QQmlEngine>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QSet>
#include <QStringList>


class ExtractionStatusPoller : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ExtractionStatusPoller(QObject *parent = Q_NULLPTR);
    Q_INVOKABLE void startPolling(int jobId);

signals:
    void completed();
    void failed(const QString &reason);
    void progressUpdated(int currentStep, int totalSteps, const QString &groupLabel, double progressPct);
    void missingMetricsReceived(const QString &groupLabel, const QStringList &missingKeys);

private:
    void poll();

    static constexpr int PollIntervalMs = 1000;

    QNetworkAccessManager _network;
    QTimer _timer;
    int _jobId = -1;
    QSet<QString> _reportedMissingSections;
    int _lastStep{-1};
    QString _lastGroup;
    Logger _log{"QmlClient.ExtractionStatusPoller"};
};

#endif // EXTRACTION_STATUS_POLLER_H
