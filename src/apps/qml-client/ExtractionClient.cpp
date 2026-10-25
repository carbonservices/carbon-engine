#include "ExtractionClient.h"

// Qt
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

ExtractionClient::ExtractionClient(QObject *parent) : ApiClientBase(parent) {}

void ExtractionClient::startExtraction(int jobId)
{
    _log.info(QString("Starting extraction for job %1").arg(jobId));

    QNetworkRequest request = createJsonRequest(endpoint(ApiRoutes::ExtractionStart));
    QJsonDocument body(QJsonObject{{"job_id", jobId}});
    QNetworkReply *reply = network().post(request, body.toJson());

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

void ExtractionClient::cancelExtraction(int jobId)
{
    QJsonObject reqObj;
    reqObj["job_id"] = jobId;

    QNetworkRequest req = createJsonRequest(endpoint(ApiRoutes::ExtractionCancel));
    QByteArray body = QJsonDocument(reqObj).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = network().post(req, body);

    connect(reply, &QNetworkReply::finished, this, [this, reply, jobId]()
    {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            _log.error(QString("Cancel extraction failed for job %1: %2").arg(jobId).arg(reply->errorString()));
            emit requestFailed("Failed to cancel extraction: " + reply->errorString());
            return;
        }
        _log.info(QString("Extraction cancelled successfully for job %1").arg(jobId));
        emit extractionCancelled(jobId);
    });
}

void ExtractionClient::fetchSections(int jobId)
{
    QUrl url = endpoint(ApiRoutes::ExtractionSections);
    QUrlQuery query;
    query.addQueryItem("job_id", QString::number(jobId));
    url.setQuery(query);

    QNetworkReply *reply = network().get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished, this, [this, reply, jobId]()
    {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            _log.error(QString("Fetching sections failed for job %1: %2").arg(jobId).arg(reply->errorString()));
            emit requestFailed(reply->errorString());
            return;
        }

        auto doc = QJsonDocument::fromJson(reply->readAll());
        emit sectionsReady(doc.object().value("sections").toArray(),
                           doc.object().value("warnings").toArray(),
                           doc.object().value("issues").toArray());
    });
}

void ExtractionClient::fetchSchema()
{
    QUrl url = endpoint(ApiRoutes::SchemaBrsr);
    QNetworkReply *reply = network().get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            _log.error(QString("Fetching schema failed: %1").arg(reply->errorString()));
            emit requestFailed(reply->errorString());
            return;
        }

        auto doc = QJsonDocument::fromJson(reply->readAll());
        emit schemaReady(doc.object());
    });
}

void ExtractionClient::updateSection(int jobId, const QString &groupId, const QJsonObject &data)
{
    QNetworkRequest request = createJsonRequest(endpoint(ApiRoutes::ExtractionSections));
    QJsonDocument body(QJsonObject{{"job_id", jobId}, {"group_id", groupId}, {"data", data}});
    QNetworkReply *reply = network().sendCustomRequest(request, "PUT", body.toJson());

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
    QNetworkRequest request = createJsonRequest(endpoint(ApiRoutes::ExtractionAccept));
    QJsonDocument body(QJsonObject{{"job_id", jobId}});
    QNetworkReply *reply = network().post(request, body.toJson());

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
