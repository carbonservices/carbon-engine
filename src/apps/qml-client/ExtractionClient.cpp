#include "ExtractionClient.h"

// Qt
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>


ExtractionClient::ExtractionClient(QObject *parent) : QObject(parent) {}

void ExtractionClient::startExtraction(int jobId)
{
    _log.info(QString("Starting extraction for job %1").arg(jobId));

    QNetworkRequest request(ExtractionStartEndpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonDocument body(QJsonObject{{"job_id", jobId}});
    QNetworkReply *reply = _network.post(request, body.toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, jobId]()
    {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            _log.error(QString("Extraction start failed for job %1: %2").arg(jobId).arg(reply->errorString()));
            emit requestFailed(reply->errorString());
            return;
        }
        emit extractionStarted();
    });
}

void ExtractionClient::fetchSections(int jobId)
{
    QUrl url = ExtractionSectionsEndpoint;
    QUrlQuery query;
    query.addQueryItem("job_id", QString::number(jobId));
    url.setQuery(query);

    QNetworkReply *reply = _network.get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished, this, [this, reply, jobId]()
    {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            _log.error(QString("Fetching sections failed for job %1: %2").arg(jobId).arg(reply->errorString()));
            emit requestFailed(reply->errorString());
            return;
        }

        auto doc = QJsonDocument::fromJson(reply->readAll());
        emit sectionsReady(doc.object().value("sections").toArray(), doc.object().value("warnings").toArray());
    });
}

void ExtractionClient::updateSection(int jobId, const QString &groupId, const QJsonObject &data)
{
    QNetworkRequest request(ExtractionSectionsEndpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonDocument body(QJsonObject{{"job_id", jobId}, {"group_id", groupId}, {"data", data}});
    QNetworkReply *reply = _network.sendCustomRequest(request, "PUT", body.toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, groupId]()
    {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            _log.error(QString("Section update failed for group %1: %2").arg(groupId).arg(reply->errorString()));
            emit requestFailed(reply->errorString());
            return;
        }
        emit updateSucceeded(groupId);
    });
}

void ExtractionClient::acceptReport(int jobId)
{
    QNetworkRequest request(ExtractionAcceptEndpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonDocument body(QJsonObject{{"job_id", jobId}});
    QNetworkReply *reply = _network.post(request, body.toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, jobId]()
    {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            _log.error(QString("Accept failed for job %1: %2").arg(jobId).arg(reply->errorString()));
            emit requestFailed(reply->errorString());
            return;
        }
        emit acceptSucceeded();
    });
}
