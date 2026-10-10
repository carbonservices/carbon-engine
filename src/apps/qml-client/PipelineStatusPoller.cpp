#include "PipelineStatusPoller.h"

// Qt
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

PipelineStatusPoller::PipelineStatusPoller(QObject *parent) : QObject(parent)
{
    connect(&_timer, &QTimer::timeout, this, &PipelineStatusPoller::poll);
}

void PipelineStatusPoller::startPolling(int jobId)
{
    _jobId = jobId;
    _log.info(QString("Started polling job %1").arg(jobId));
    poll();
    _timer.start(PollIntervalMs);
}

void PipelineStatusPoller::poll()
{
    QUrl url("http://localhost:8080/api/jobs");
    QUrlQuery query;
    query.addQueryItem("job_id", QString::number(_jobId));
    url.setQuery(query);

    QNetworkReply *reply = _network.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
        {
            _log.error(QString("Poll failed for job %1: %2").arg(_jobId).arg(reply->errorString()));
            return;
        }

        auto doc = QJsonDocument::fromJson(reply->readAll());
        QString status = doc.object().value("status").toString();
        QJsonArray docs = doc.object().value("documents").toArray();
        _log.info(QString("Job %1 status: %2, %3 document(s)").arg(_jobId).arg(status).arg(docs.size()));

        emit statusUpdated(status, docs);

        if (status == "COMPLETED")
        {
            _timer.stop();
            emit completed();
        }
        else if (status == "FAILED")
        {
            _timer.stop();
            emit failed("Ingestion failed");
        }
    });
}
