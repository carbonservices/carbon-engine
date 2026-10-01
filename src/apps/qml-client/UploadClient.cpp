#include "UploadClient.h"

// Qt
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>


UploadClient::UploadClient(QObject *parent) : QObject(parent) {}

void UploadClient::uploadDocument(const QUrl &filePath)
{
    const QString localPath = filePath.toLocalFile();
    _log.info(QString("Uploading document: %1").arg(localPath));

    QNetworkRequest request(IngestEndpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonDocument body(QJsonObject{{"file_path", localPath}});
    QNetworkReply *reply = _network.post(request, body.toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, localPath]()
    {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            _log.error(QString("Upload failed for %1: %2").arg(localPath, reply->errorString()));
            emit uploadFailed(reply->errorString());
            return;
        }

        auto doc = QJsonDocument::fromJson(reply->readAll());
        int jobId = doc.object().value("job_id").toInt(-1);
        _log.info(QString("Upload queued for %1, job_id=%2").arg(localPath).arg(jobId));
        emit jobQueued(jobId);
    });
}
