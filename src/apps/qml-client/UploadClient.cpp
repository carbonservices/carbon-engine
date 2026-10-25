#include "UploadClient.h"

// Qt
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDir>

UploadClient::UploadClient(QObject *parent) : ApiClientBase(parent) {}

void UploadClient::uploadDocuments(const QVariantList &filePaths)
{
    QStringList localPaths;
    for (const QVariant& item : filePaths) {
        QString pathStr;
        if (item.canConvert<QUrl>()) {
            QUrl url = item.toUrl();
            if (url.isLocalFile()) {
                pathStr = url.toLocalFile();
            } else {
                pathStr = url.toString();
            }
        } else {
            pathStr = item.toString();
        }

        if (pathStr.startsWith("file://", Qt::CaseInsensitive)) {
            QUrl url(pathStr);
            if (url.isLocalFile()) {
                pathStr = url.toLocalFile();
            } else if (pathStr.startsWith("file:///", Qt::CaseInsensitive)) {
                pathStr = pathStr.mid(8);
            } else {
                pathStr = pathStr.mid(7);
            }
        }

        pathStr = QDir::cleanPath(pathStr);
        if (!pathStr.isEmpty()) {
            localPaths.append(pathStr);
        }
    }

    if (localPaths.isEmpty()) {
        _log.error("Upload aborted: No valid local file paths provided");
        emit uploadFailed("No valid local file paths provided");
        return;
    }
    _log.info(QString("Uploading %1 document(s): %2").arg(localPaths.size()).arg(localPaths.join(", ")));

    QNetworkRequest request = createJsonRequest(endpoint(ApiRoutes::Ingest));
    QJsonDocument body(QJsonObject{{"file_paths", QJsonArray::fromStringList(localPaths)}});
    QNetworkReply *reply = network().post(request, body.toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, localPaths]()
    {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            _log.error(QString("Upload failed for %1 document(s): %2").arg(localPaths.size()).arg(reply->errorString()));
            emit uploadFailed(reply->errorString());
            return;
        }

        auto doc = QJsonDocument::fromJson(reply->readAll());
        int jobId = doc.object().value("job_id").toInt(-1);
        if (jobId < 0) {
            QString err = doc.object().value("error").toString("Server failed to assign valid job ID");
            _log.error(QString("Upload failed to produce valid job_id: %1").arg(err));
            emit uploadFailed(err);
            return;
        }
        _log.info(QString("Upload queued for %1 document(s), job_id=%2").arg(localPaths.size()).arg(jobId));
        emit jobQueued(jobId);
    });
}

void UploadClient::cancelJob(int jobId)
{
    QJsonObject reqObj;
    reqObj["job_id"] = jobId;

    QNetworkRequest request = createJsonRequest(endpoint(ApiRoutes::JobCancel));
    QJsonDocument body(reqObj);
    QNetworkReply *reply = network().post(request, body.toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, jobId]()
    {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            _log.error(QString("Cancel job failed for job %1: %2").arg(jobId).arg(reply->errorString()));
            emit uploadFailed("Failed to cancel job: " + reply->errorString());
            return;
        }
        _log.info(QString("Job %1 cancelled successfully").arg(jobId));
        emit jobCancelled(jobId);
    });
}
