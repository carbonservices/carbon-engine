#ifndef EXTRACTION_CLIENT_H
#define EXTRACTION_CLIENT_H

#include "ApiClientBase.h"

// Qt
#include <QQmlEngine>
#include <QJsonArray>
#include <QJsonObject>

class ExtractionClient : public ApiClientBase
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ExtractionClient(QObject* parent = nullptr);

    Q_INVOKABLE void startExtraction(int jobId);
    Q_INVOKABLE void cancelExtraction(int jobId);
    Q_INVOKABLE void fetchSections(int jobId);
    Q_INVOKABLE void fetchSchema();
    Q_INVOKABLE void updateSection(int jobId, const QString& groupId, const QJsonObject& data);
    Q_INVOKABLE void acceptReport(int jobId);

signals:
    void extractionStarted();
    void extractionCancelled(int jobId);
    void sectionsReady(const QJsonArray& sections, const QJsonArray& warnings, const QJsonArray& issues);
    void schemaReady(const QJsonObject& schema);
    void updateSucceeded(const QString& groupId);
    void acceptSucceeded();
    void requestFailed(const QString& reason);

private:
    Logger _log{"QmlClient.ExtractionClient"};
};

#endif // EXTRACTION_CLIENT_H
