#ifndef UPLOAD_CLIENT_H
#define UPLOAD_CLIENT_H

#include "ApiClientBase.h"

// Qt
#include <QQmlEngine>
#include <QVariantList>

class UploadClient : public ApiClientBase
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit UploadClient(QObject* parent = nullptr);
    Q_INVOKABLE void uploadDocuments(const QVariantList& filePaths);
    Q_INVOKABLE void cancelJob(int jobId);

signals:
    void jobQueued(int jobId);
    void jobCancelled(int jobId);
    void uploadFailed(const QString& reason);

private:
    Logger _log{"QmlClient.UploadClient"};
};

#endif // UPLOAD_CLIENT_H
