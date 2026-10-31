#include "ExtractionStatusPoller.h"

// Qt
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

static QVariantList parseMissingMetrics(const QJsonObject& root)
{
    QJsonArray missingArr = root.value("missing_metrics").toArray();
    QVariantList fullList;
    for (const auto& item : missingArr) {
        QJsonObject secObj = item.toObject();
        QString label = secObj.value("group_label").toString();
        QJsonArray keysJsonArr = secObj.value("missing_keys").toArray();
        QVariantList missingKeys;
        for (const auto& k : keysJsonArr) {
            missingKeys.append(k.toString());
        }
        if (!missingKeys.isEmpty()) {
            QVariantMap m;
            m["group"] = label;
            m["missing"] = missingKeys;
            fullList.append(m);
        }
    }
    return fullList;
}

ExtractionStatusPoller::ExtractionStatusPoller(QObject *parent) : ApiClientBase(parent)
{
    connect(&_timer, &QTimer::timeout, this, &ExtractionStatusPoller::poll);
}

void ExtractionStatusPoller::startPolling(int jobId)
{
    stopPolling();
    _jobId = jobId;
    _reportedMissingSections.clear();
    _lastStep = CoreConstants::StepUninitialized;
    _lastGroup.clear();
    _log.info(QString("Started polling extraction for job %1").arg(jobId));
    poll();
    _timer.start(PollIntervalMs);
}

void ExtractionStatusPoller::stopPolling()
{
    _log.info(QString("Stopped polling extraction for job %1").arg(_jobId));
    _timer.stop();
    _jobId = CoreConstants::InvalidJobId;
    if (_activeReply) {
        _activeReply->abort();
        _activeReply->deleteLater();
        _activeReply = nullptr;
    }
}

void ExtractionStatusPoller::fetchStatusOnly(int jobId)
{
    if (jobId < 0) return;
    if (_fetchReply) {
        _fetchReply->abort();
        _fetchReply->deleteLater();
        _fetchReply = nullptr;
    }

    QUrl url = endpoint(ApiRoutes::ExtractionStatus);
    QUrlQuery query;
    query.addQueryItem("job_id", QString::number(jobId));
    url.setQuery(query);

    QNetworkReply *reply = network().get(QNetworkRequest(url));
    reply->setProperty("polledJobId", jobId);
    _fetchReply = reply;
    connect(reply, &QNetworkReply::finished, this, &ExtractionStatusPoller::onFetchStatusFinished);
}

void ExtractionStatusPoller::onFetchStatusFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    if (_fetchReply == reply) {
        _fetchReply = nullptr;
    }
    reply->deleteLater();
    int polledJobId = reply->property("polledJobId").toInt();

    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() != QNetworkReply::OperationCanceledError) {
            _log.error(QString("Fetch status failed for job %1: %2").arg(polledJobId).arg(reply->errorString()));
        }
        return;
    }

    auto doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject root = doc.object();
    QVariantList fullList = parseMissingMetrics(root);
    emit missingMetricsUpdated(fullList);
    _log.info(QString("Job %1 on-demand status fetched: missing %2 sections").arg(polledJobId).arg(fullList.size()));
}

void ExtractionStatusPoller::poll()
{
    int polledJobId = _jobId;
    if (polledJobId < 0 || _activeReply) {
        return;
    }

    QUrl url = endpoint(ApiRoutes::ExtractionStatus);
    QUrlQuery query;
    query.addQueryItem("job_id", QString::number(polledJobId));
    url.setQuery(query);

    QNetworkReply *reply = network().get(QNetworkRequest(url));
    reply->setProperty("polledJobId", polledJobId);
    _activeReply = reply;
    connect(reply, &QNetworkReply::finished, this, &ExtractionStatusPoller::onPollFinished);
}

void ExtractionStatusPoller::onPollFinished()
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
            _log.error(QString("Extraction poll failed for job %1: %2").arg(_jobId).arg(reply->errorString()));
        }
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

    QVariantList fullList = parseMissingMetrics(root);
    for (const auto& item : fullList) {
        QVariantMap m = item.toMap();
        QString label = m.value("group").toString();
        QVariantList missingKeysList = m.value("missing").toList();
        QStringList missingKeysStrings;
        for (const auto& k : missingKeysList) {
            missingKeysStrings.append(k.toString());
        }
        if (!_reportedMissingSections.contains(label)) {
            _reportedMissingSections.insert(label);
            emit missingMetricsReceived(label, missingKeysStrings);
        }
    }
    emit missingMetricsUpdated(fullList);

    _log.info(QString("Job %1 extraction status: %2 (step %3/%4: %5, missing %6 sections)")
        .arg(polledJobId).arg(status).arg(step).arg(total).arg(currentGroup).arg(fullList.size()));

    if (status == ExtractionStatus::ReadyForReview ||
        status == ExtractionStatus::Completed ||
        status == ExtractionStatus::Accepted)
    {
        _timer.stop();
        _jobId = CoreConstants::InvalidJobId;
        emit completed();
    }
    else if (status == ExtractionStatus::Failed)
    {
        _timer.stop();
        _jobId = CoreConstants::InvalidJobId;
        emit failed("Extraction failed");
    }
}
