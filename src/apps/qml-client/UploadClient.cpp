#include "UploadClient.h"

// Qt
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QDir>

UploadClient::UploadClient(QObject *parent) : ApiClientBase(parent) {}

void UploadClient::uploadDocuments(const QVariantList &filePaths)
{
    if (filePaths.isEmpty())
    {
        _log.warn("uploadDocuments invoked with empty path list");
        emit uploadFailed("No documents selected");
        return;
    }

    QStringList localPaths;
    for (const auto &item : filePaths)
    {
        QString pathStr = item.toString().trimmed();
        if (pathStr.startsWith("file:///"))
        {
#ifdef _WIN32
            pathStr = pathStr.mid(8);
#else
            pathStr = pathStr.mid(7);
#endif
        }
        else if (pathStr.startsWith("file://"))
        {
            pathStr = pathStr.mid(7);
        }

        pathStr = QDir::toNativeSeparators(pathStr);
        if (!pathStr.isEmpty())
        {
            localPaths.append(pathStr);
        }
    }

    if (localPaths.isEmpty())
    {
        _log.warn("uploadDocuments: parsed path list is empty");
        emit uploadFailed("Valid local document paths could not be determined");
        return;
    }
    _log.info(QString("Uploading %1 document(s): %2").arg(localPaths.size()).arg(localPaths.join(", ")));

    QNetworkRequest request = createJsonRequest(endpoint(ApiRoutes::Ingest));
    QJsonDocument body(QJsonObject{{"file_paths", QJsonArray::fromStringList(localPaths)}});
    QNetworkReply *reply = network().post(request, body.toJson());
    reply->setProperty("docCount", localPaths.size());

    connect(reply, &QNetworkReply::finished, this, &UploadClient::onUploadFinished);
}

void UploadClient::onUploadFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    int docCount = reply->property("docCount").toInt();

    if (reply->error() != QNetworkReply::NoError) {
        _log.error(QString("Upload failed for %1 document(s): %2").arg(docCount).arg(reply->errorString()));
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
    _log.info(QString("Upload queued for %1 document(s), job_id=%2").arg(docCount).arg(jobId));
    emit jobQueued(jobId);
}

void UploadClient::cancelJob(int jobId)
{
    QJsonObject reqObj;
    reqObj["job_id"] = jobId;

    QNetworkRequest request = createJsonRequest(endpoint(ApiRoutes::JobCancel));
    QJsonDocument body(reqObj);
    QNetworkReply *reply = network().post(request, body.toJson());
    reply->setProperty("jobId", jobId);

    connect(reply, &QNetworkReply::finished, this, &UploadClient::onCancelJobFinished);
}

void UploadClient::onCancelJobFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    int jobId = reply->property("jobId").toInt();

    if (reply->error() != QNetworkReply::NoError) {
        _log.error(QString("Cancel job failed for job %1: %2").arg(jobId).arg(reply->errorString()));
        emit uploadFailed("Failed to cancel job: " + reply->errorString());
        return;
    }
    _log.info(QString("Job %1 cancelled successfully").arg(jobId));
    emit jobCancelled(jobId);
}
