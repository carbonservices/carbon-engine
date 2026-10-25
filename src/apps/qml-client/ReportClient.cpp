#include "ReportClient.h"

// Qt
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QFile>
#include <QTextStream>

ReportClient::ReportClient(QObject *parent) : ApiClientBase(parent) {}

void ReportClient::fetchReport(int jobId)
{
    _log.info(QString("Fetching report for job %1").arg(jobId));

    QNetworkRequest request = createJsonRequest(endpoint(ApiRoutes::ReportExport));
    QJsonDocument body(QJsonObject{{"job_id", jobId}});
    QNetworkReply *reply = network().post(request, body.toJson());
    reply->setProperty("jobId", jobId);

    connect(reply, &QNetworkReply::finished, this, &ReportClient::onFetchReportFinished);
}

void ReportClient::onFetchReportFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    int jobId = reply->property("jobId").toInt();

    if (reply->error() != QNetworkReply::NoError) {
        _log.error(QString("Report fetch failed for job %1: %2").arg(jobId).arg(reply->errorString()));
        emit reportFailed(reply->errorString());
        return;
    }

    QByteArray rawData = reply->readAll();
    auto doc = QJsonDocument::fromJson(rawData);
    QString htmlContent;
    if (doc.isObject() && doc.object().contains("report")) {
        htmlContent = doc.object().value("report").toString();
    } else {
        htmlContent = QString::fromUtf8(rawData);
    }

    _log.info(QString("Report fetch completed for job %1 (%2 bytes)").arg(jobId).arg(htmlContent.length()));
    emit reportReady(htmlContent);
}

void ReportClient::fetchReports()
{
    _log.info("Fetching reports list from server");

    QNetworkRequest request(endpoint(ApiRoutes::Reports));
    QNetworkReply *reply = network().get(request);

    connect(reply, &QNetworkReply::finished, this, &ReportClient::onFetchReportsFinished);
}

void ReportClient::onFetchReportsFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        _log.error(QString("Failed to fetch reports list: %1").arg(reply->errorString()));
        return;
    }

    auto doc = QJsonDocument::fromJson(reply->readAll());
    if (doc.isArray()) {
        _log.info(QString("Fetched %1 reports from database").arg(doc.array().size()));
        emit reportsReceived(doc.array());
    }
}

void ReportClient::deleteReport(int jobId)
{
    _log.info(QString("Requesting deletion of report / job %1").arg(jobId));

    QUrl url = endpoint(ApiRoutes::Reports);
    QUrlQuery query;
    query.addQueryItem("job_id", QString::number(jobId));
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = network().deleteResource(request);
    reply->setProperty("jobId", jobId);

    connect(reply, &QNetworkReply::finished, this, &ReportClient::onDeleteReportFinished);
}

void ReportClient::onDeleteReportFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    int jobId = reply->property("jobId").toInt();

    if (reply->error() != QNetworkReply::NoError) {
        _log.error(QString("Failed to delete report %1: %2").arg(jobId).arg(reply->errorString()));
        emit reportDeleteFailed(reply->errorString());
        return;
    }

    _log.info(QString("Successfully deleted report %1").arg(jobId));
    emit reportDeleted(jobId);
}

void ReportClient::fetchDocuments(int jobId)
{
    _log.info(QString("Fetching documents for job %1").arg(jobId));

    QUrl url = endpoint(ApiRoutes::Documents);
    QUrlQuery query;
    query.addQueryItem("job_id", QString::number(jobId));
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = network().get(request);
    reply->setProperty("jobId", jobId);

    connect(reply, &QNetworkReply::finished, this, &ReportClient::onFetchDocumentsFinished);
}

void ReportClient::onFetchDocumentsFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    int jobId = reply->property("jobId").toInt();

    if (reply->error() != QNetworkReply::NoError) {
        _log.error(QString("Failed to fetch documents for job %1: %2").arg(jobId).arg(reply->errorString()));
        return;
    }

    auto doc = QJsonDocument::fromJson(reply->readAll());
    if (doc.isObject()) {
        QJsonArray docs = doc.object().value("documents").toArray();
        _log.info(QString("Fetched %1 documents for job %2").arg(docs.size()).arg(jobId));
        emit documentsReceived(jobId, docs);
    }
}

bool ReportClient::saveHtmlToFile(const QString &filePath, const QString &content)
{
    QString localPath = filePath;
    if (localPath.startsWith("file:///")) {
#ifdef _WIN32
        localPath = localPath.mid(8);
#else
        localPath = localPath.mid(7);
#endif
    } else if (localPath.startsWith("file://")) {
        localPath = localPath.mid(7);
    }

    QFile file(localPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        _log.error(QString("Failed to open file for writing: %1").arg(localPath));
        return false;
    }

    QTextStream out(&file);
    out << content;
    file.close();
    _log.info(QString("Report saved successfully to: %1").arg(localPath));
    return true;
}
