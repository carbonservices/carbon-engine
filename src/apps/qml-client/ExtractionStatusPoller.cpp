#include "ExtractionStatusPoller.h"

// Qt
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

ExtractionStatusPoller::ExtractionStatusPoller(QObject *parent) : ApiClientBase(parent)
{
    connect(&_timer, &QTimer::timeout, this, &ExtractionStatusPoller::poll);
}

void ExtractionStatusPoller::startPolling(int jobId)
{
    _jobId = jobId;
    _reportedMissingSections.clear();
    _lastStep = -1;
    _lastGroup.clear();
    _log.info(QString("Started polling extraction for job %1").arg(jobId));
    poll();
    _timer.start(PollIntervalMs);
}

void ExtractionStatusPoller::stopPolling()
{
    _log.info(QString("Stopped polling extraction for job %1").arg(_jobId));
    _timer.stop();
    _jobId = -1;
}

void ExtractionStatusPoller::poll()
{
    int polledJobId = _jobId;
    if (polledJobId < 0) {
        return;
    }

    QUrl url = endpoint(ApiRoutes::ExtractionStatus);
    QUrlQuery query;
    query.addQueryItem("job_id", QString::number(polledJobId));
    url.setQuery(query);

    QNetworkReply *reply = network().get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, polledJobId]()
    {
        reply->deleteLater();
        if (_jobId != polledJobId || _jobId < 0) {
            return;
        }
        if (reply->error() != QNetworkReply::NoError)
        {
            _log.error(QString("Extraction poll failed for job %1: %2").arg(_jobId).arg(reply->errorString()));
            return;
        }

        auto doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject root = doc.object();
        QString status = root.value("status").toString();

        int step = root.value("current_step").toInt(0);
        int total = root.value("total_steps").toInt(0);
        QString currentGroup = root.value("current_group").toString();
        double pct = root.value("progress_pct").toDouble(0.0);

        if (total > 0 && (step != _lastStep || currentGroup != _lastGroup)) {
            _lastStep = step;
            _lastGroup = currentGroup;
            emit progressUpdated(step, total, currentGroup, pct);
        }

        QJsonArray missingArr = root.value("missing_metrics").toArray();
        for (const auto& item : missingArr) {
            QJsonObject secObj = item.toObject();
            QString label = secObj.value("group_label").toString();
            if (!_reportedMissingSections.contains(label)) {
                _reportedMissingSections.insert(label);
                QStringList missingKeys;
                for (const auto& k : secObj.value("missing_keys").toArray()) {
                    missingKeys.append(k.toString());
                }
                if (!missingKeys.isEmpty()) {
                    emit missingMetricsReceived(label, missingKeys);
                }
            }
        }

        _log.info(QString("Job %1 extraction status: %2 (step %3/%4: %5)").arg(_jobId).arg(status).arg(step).arg(total).arg(currentGroup));

        if (status == ExtractionStatus::ReadyForReview ||
            status == ExtractionStatus::Completed ||
            status == ExtractionStatus::Accepted)
        {
            _timer.stop();
            emit completed();
        }
        else if (status == ExtractionStatus::Failed)
        {
            _timer.stop();
            emit failed("Extraction failed");
        }
        else if (status == ExtractionStatus::Cancelled)
        {
            _timer.stop();
            emit cancelled();
        }
    });
}
