#ifndef REPORT_CLIENT_H
#define REPORT_CLIENT_H

#include "ApiClientBase.h"

// Qt
#include <QQmlEngine>
#include <QJsonArray>

class ReportClient : public ApiClientBase
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ReportClient(QObject* parent = nullptr);
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
    Logger _log{"QmlClient.ReportClient"};
};

#endif // REPORT_CLIENT_H
