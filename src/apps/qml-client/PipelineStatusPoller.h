#ifndef PIPELINE_STATUS_POLLER_H
#define PIPELINE_STATUS_POLLER_H

#include "logging/Logger.h"

// Qt
#include <QObject>
#include <QQmlEngine>
#include <QAbstractListModel>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QVector>


enum class StageStatus
{
    Pending,
    Active,
    Done,
    Failed
};

struct PipelineStage
{
    QString name;
    StageStatus status;
};

class PipelineStatusPoller : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum Role
    {
        NameRole = Qt::UserRole + 1,
        StatusRole
    };

    explicit PipelineStatusPoller(QObject *parent = Q_NULLPTR);
    Q_INVOKABLE void startPolling(int jobId);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void completed();
    void failed(const QString &reason);

private:
    void poll();
    void setStage(int index, StageStatus status);

    static constexpr int PollIntervalMs = 1000;
    static inline const QVector<QString> StageNames = {"Queued", "Processing", "Completed"};

    QNetworkAccessManager _network;
    QTimer _timer;
    QVector<PipelineStage> _stages;
    int _jobId = -1;
    Logger _log{"QmlClient.PipelineStatusPoller"};
};

#endif // PIPELINE_STATUS_POLLER_H