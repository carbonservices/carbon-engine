#include "ReportClient.h"

// Qt
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QFile>
#include <QTextStream>
#include <memory>

ReportClient::ReportClient(QObject *parent) : ApiClientBase(parent) {}

void ReportClient::fetchReport(int jobId)
{
    _log.info(QString("Fetching report for job %1").arg(jobId));

    QNetworkRequest request = createJsonRequest(endpoint(ApiRoutes::ReportExport));
    QJsonDocument body(QJsonObject{{"job_id", jobId}});
    QNetworkReply *reply = network().post(request, body.toJson());

    auto buffer = std::make_shared<QByteArray>();

    connect(reply, &QNetworkReply::readyRead, this, [reply, buffer]()
    {
        buffer->append(reply->readAll());
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, jobId, buffer]()
    {
        buffer->append(reply->readAll());
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            _log.error(QString("Report fetch failed for job %1: %2").arg(jobId).arg(reply->errorString()));
            emit reportFailed(reply->errorString());
            return;
        }

        auto doc = QJsonDocument::fromJson(*buffer);
        QString htmlContent;
        if (doc.isObject() && doc.object().contains("report")) {
            htmlContent = doc.object().value("report").toString();
        } else {
            htmlContent = QString::fromUtf8(*buffer);
        }

        _log.info(QString("Report fetch completed for job %1 (%2 bytes)").arg(jobId).arg(htmlContent.length()));
        emit reportReady(htmlContent);
    });
}

void ReportClient::fetchReports()
{
    _log.info("Fetching reports list from server");

    QNetworkRequest request(endpoint(ApiRoutes::Reports));
    QNetworkReply *reply = network().get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
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
    });
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

    connect(reply, &QNetworkReply::finished, this, [this, reply, jobId]()
    {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            _log.error(QString("Failed to delete report %1: %2").arg(jobId).arg(reply->errorString()));
            emit reportDeleteFailed(reply->errorString());
            return;
        }

        _log.info(QString("Successfully deleted report %1").arg(jobId));
        emit reportDeleted(jobId);
    });
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

    connect(reply, &QNetworkReply::finished, this, [this, reply, jobId]()
    {
        reply->deleteLater();
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
    });
}

bool ReportClient::saveHtmlToFile(const QString& filePath, const QString& content)
{
    QUrl url(filePath);
    const QString localPath = url.isLocalFile() ? url.toLocalFile() : filePath;
    QFile file(localPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        _log.error(QString("Failed to open file for writing at %1: %2").arg(localPath, file.errorString()));
        return false;
    }
    QTextStream out(&file);
    out << content;
    file.close();
    _log.info(QString("Successfully exported report HTML to %1").arg(localPath));
    return true;
}
