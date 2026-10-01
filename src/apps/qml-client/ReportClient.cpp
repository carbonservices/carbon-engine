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

    connect(reply, &QNetworkReply::readyRead, this, [this, reply]()
    {
        QByteArray chunk = reply->readAll();
        emit reportChunkReceived(QString::fromUtf8(chunk));
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, jobId]()
    {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            _log.error(QString("Report fetch failed for job %1: %2").arg(jobId).arg(reply->errorString()));
            emit reportFailed(reply->errorString());
            return;
        }
        _log.info(QString("Report stream completed for job %1").arg(jobId));
    });
}
