#include "ReportClient.h"

// Qt
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>


ReportClient::ReportClient(QObject *parent) : QObject(parent) {}

void ReportClient::fetchReport(int jobId)
{
    _log.info(QString("Fetching report for job %1").arg(jobId));

    QNetworkRequest request(ReportEndpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonDocument body(QJsonObject{{"job_id", jobId}});
    QNetworkReply *reply = _network.post(request, body.toJson());

    QByteArray* buffer = new QByteArray();

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
            delete buffer;
            return;
        }

        auto doc = QJsonDocument::fromJson(*buffer);
        QString htmlContent;
        if (doc.isObject() && doc.object().contains("report")) {
            htmlContent = doc.object().value("report").toString();
        } else {
            htmlContent = QString::fromUtf8(*buffer);
        }

        delete buffer;

        _log.info(QString("Report fetch completed for job %1 (%2 bytes)").arg(jobId).arg(htmlContent.length()));
        emit reportReady(htmlContent);
    });
}
