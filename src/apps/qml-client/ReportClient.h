#ifndef REPORT_CLIENT_H
#define REPORT_CLIENT_H

#include "logging/Logger.h"

// Qt
#include <QObject>
#include <QQmlEngine>
#include <QNetworkAccessManager>
#include <QUrl>


class ReportClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ReportClient(QObject* parent = Q_NULLPTR);
    Q_INVOKABLE void fetchReport(int jobId);

signals:
    void reportReady(const QString& report);
    void reportChunkReceived(const QString& chunk);
    void reportFailed(const QString& reason);

private:
    static inline const QUrl ReportEndpoint{"http://localhost:8080/api/report"};

    QNetworkAccessManager _network;
    Logger _log{"QmlClient.ReportClient"};
};

#endif // REPORT_CLIENT_H
