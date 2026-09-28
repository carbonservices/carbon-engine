#ifndef UPLOAD_CLIENT_H
#define UPLOAD_CLIENT_H

#include "logging/Logger.h"

// Qt
#include <QNetworkAccessManager>
#include <QObject>
#include <QQmlEngine>
#include <QUrl>

class UploadClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit UploadClient(QObject *parent = Q_NULLPTR);
    Q_INVOKABLE void uploadDocument(const QUrl &filePath);

signals:
    void jobQueued(int jobId);
    void uploadFailed(const QString &reason);

private:
    QNetworkAccessManager           _network;
    Logger                          _log{"QmlClient.UploadClient"};
};

#endif // UPLOAD_CLIENT_H
