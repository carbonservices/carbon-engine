#ifndef REPORT_CLIENT_H
#define REPORT_CLIENT_H

#include "logging/Logger.h"

// Qt
#include <QNetworkAccessManager>
#include <QObject>
#include <QQmlEngine>

class ReportClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ReportClient(QObject *parent = nullptr);

    Q_INVOKABLE void fetchReport(int jobId);

signals:
    void reportReady(const QString &report);
    void reportChunkReceived(const QString &chunk);
    void reportFailed(const QString &reason);

private:
    QNetworkAccessManager _network;
    Logger _log{"QmlClient.ReportClient"};
};

#endif // REPORT_CLIENT_H
