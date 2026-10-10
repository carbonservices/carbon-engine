#ifndef REPORT_CLIENT_H
#define REPORT_CLIENT_H

#include "logging/Logger.h"

// Qt
#include <QObject>
#include <QQmlEngine>
#include <QNetworkAccessManager>
#include <QUrl>


#include <QJsonArray>

class ReportClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ReportClient(QObject* parent = Q_NULLPTR);
    Q_INVOKABLE void fetchReport(int jobId);
    Q_INVOKABLE void fetchReports();
    Q_INVOKABLE void fetchDocuments(int jobId);
    Q_INVOKABLE void deleteReport(int jobId);
    Q_INVOKABLE bool saveHtmlToFile(const QString& filePath, const QString& content);

signals:
    void reportReady(const QString& report);
    void reportChunkReceived(const QString& chunk);
    void reportFailed(const QString& reason);
    void reportsReceived(const QJsonArray& reports);
    void documentsReceived(int jobId, const QJsonArray& documents);
    void reportDeleted(int jobId);
    void reportDeleteFailed(const QString& reason);

private:
    static inline const QUrl ReportEndpoint{"http://localhost:8080/api/report/export"};
    static inline const QUrl ReportsEndpoint{"http://localhost:8080/api/reports"};
    static inline const QUrl DocumentsEndpoint{"http://localhost:8080/api/documents"};

    QNetworkAccessManager _network;
    Logger _log{"QmlClient.ReportClient"};
};

#endif // REPORT_CLIENT_H
