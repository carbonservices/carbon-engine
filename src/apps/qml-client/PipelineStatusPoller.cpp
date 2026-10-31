#include "PipelineStatusPoller.h"

// Qt
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

PipelineStatusPoller::PipelineStatusPoller(QObject *parent) : ApiClientBase(parent)
{
    connect(&_timer, &QTimer::timeout, this, &PipelineStatusPoller::poll);
}

void PipelineStatusPoller::startPolling(int jobId)
{
    stopPolling();
    _jobId = jobId;
    _log.info(QString("Started polling job %1").arg(jobId));
    poll();
    _timer.start(PollIntervalMs);
}

void PipelineStatusPoller::stopPolling()
{
    _log.info(QString("Stopped polling job %1").arg(_jobId));
    _timer.stop();
    _jobId = CoreConstants::InvalidJobId;
    if (_activeReply) {
        _activeReply->abort();
        _activeReply->deleteLater();
        _activeReply = nullptr;
    }
}

void PipelineStatusPoller::poll()
{
    int polledJobId = _jobId;
    if (polledJobId < 0 || _activeReply) {
        return;
    }

    QUrl url = endpoint(ApiRoutes::Jobs);
    QUrlQuery query;
    query.addQueryItem("job_id", QString::number(polledJobId));
    url.setQuery(query);

    QNetworkReply *reply = network().get(QNetworkRequest(url));
    reply->setProperty("polledJobId", polledJobId);
    _activeReply = reply;
    connect(reply, &QNetworkReply::finished, this, &PipelineStatusPoller::onPollFinished);
}

void PipelineStatusPoller::onPollFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    if (_activeReply == reply) {
        _activeReply = nullptr;
    }
    reply->deleteLater();
    int polledJobId = reply->property("polledJobId").toInt();

    if (_jobId != polledJobId || _jobId < 0) {
        return;
    }
    if (reply->error() != QNetworkReply::NoError)
    {
        if (reply->error() != QNetworkReply::OperationCanceledError) {
            _log.error(QString("Poll failed for job %1: %2").arg(_jobId).arg(reply->errorString()));
        }
        return;
    }

    auto doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject root = doc.object();
    QString status = root.value("status").toString();
    QJsonArray docs = root.value("documents").toArray();

    _log.info(QString("Job %1 status: %2, %3 document(s)").arg(_jobId).arg(status).arg(docs.size()));

    emit statusUpdated(status, docs);

    if (status == JobStatus::Completed)
    {
        _timer.stop();
        _jobId = CoreConstants::InvalidJobId;
        emit completed();
    }
    else if (status == JobStatus::Failed)
    {
        _timer.stop();
        _jobId = CoreConstants::InvalidJobId;
        emit failed("Ingestion failed");
    }
    else if (status == JobStatus::Cancelled)
    {
        _timer.stop();
        _jobId = CoreConstants::InvalidJobId;
        emit cancelled();
    }
}
