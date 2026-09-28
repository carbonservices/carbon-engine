#include "PipelineStatusPoller.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

namespace
{
    constexpr int kPollIntervalMs = 1000;

    // Collapsed to match the granularity the backend actually reports (PENDING /
    // PROCESSING / COMPLETED / FAILED) - Extracting/Embedding/Indexing all fall
    // under "Processing" until JobQueue emits finer-grained status strings.
    const QVector<QString> kStageNames = {"Queued", "Processing", "Completed"};
}

PipelineStatusPoller::PipelineStatusPoller(QObject *parent) : QAbstractListModel(parent)
{
    for (const auto &name : kStageNames)
    {
        _stages.append({name, StageStatus::Pending});
    }
    connect(&_timer, &QTimer::timeout, this, &PipelineStatusPoller::poll);
}

void PipelineStatusPoller::startPolling(int jobId)
{
    _jobId = jobId;
    for (auto &stage : _stages)
    {
        stage.status = StageStatus::Pending;
    }
    emit dataChanged(index(0), index(_stages.size() - 1));
    _log.info(QString("Started polling job %1").arg(jobId));
    poll();
    _timer.start(kPollIntervalMs);
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
        if (reply->error() != QNetworkReply::NoError) {
            _log.error(QString("Poll failed for job %1: %2").arg(_jobId).arg(reply->errorString()));
            return;
        }

        auto doc = QJsonDocument::fromJson(reply->readAll());
        QString status = doc.object().value("status").toString();
        _log.info(QString("Job %1 status: %2").arg(_jobId).arg(status));

        if (status == "PENDING") {
            setStage(0, StageStatus::Active);
        } else if (status == "PROCESSING") {
            setStage(0, StageStatus::Done);
            setStage(1, StageStatus::Active);
        } else if (status == "COMPLETED") {
            setStage(0, StageStatus::Done);
            setStage(1, StageStatus::Done);
            setStage(2, StageStatus::Done);
            _timer.stop();
            emit completed();
        } else if (status == "FAILED") {
            for (int i = 0; i < _stages.size(); ++i) {
                if (_stages[i].status == StageStatus::Active) {
                    setStage(i, StageStatus::Failed);
                    break;
                }
            }
            _timer.stop();
            emit failed("Ingestion failed");
        } });
}

void PipelineStatusPoller::setStage(int stageIndex, StageStatus status)
{
    if (stageIndex < 0 || stageIndex >= _stages.size())
        return;
    if (_stages[stageIndex].status == status)
        return;
    _stages[stageIndex].status = status;
    emit dataChanged(index(stageIndex), index(stageIndex));
}

int PipelineStatusPoller::rowCount(const QModelIndex &) const
{
    return _stages.size();
}

QVariant PipelineStatusPoller::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= _stages.size())
        return {};

    const auto &stage = _stages[idx.row()];
    switch (role)
    {
    case NameRole:
        return stage.name;
    case StatusRole:
        return static_cast<int>(stage.status);
    default:
        return {};
    }
}

QHash<int, QByteArray> PipelineStatusPoller::roleNames() const
{
    return {
        {NameRole, "name"},
        {StatusRole, "status"}};
}
