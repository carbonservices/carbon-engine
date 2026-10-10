#ifndef EXTRACTION_CLIENT_H
#define EXTRACTION_CLIENT_H

#include "logging/Logger.h"

// Qt
#include <QObject>
#include <QQmlEngine>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QUrl>


class ExtractionClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ExtractionClient(QObject* parent = Q_NULLPTR);

    Q_INVOKABLE void startExtraction(int jobId);
    Q_INVOKABLE void fetchSections(int jobId);
    Q_INVOKABLE void updateSection(int jobId, const QString& groupId, const QJsonObject& data);
    Q_INVOKABLE void acceptReport(int jobId);

signals:
    void extractionStarted();
    void sectionsReady(const QJsonArray& sections, const QJsonArray& warnings);
    void updateSucceeded(const QString& groupId);
    void acceptSucceeded();
    void requestFailed(const QString& reason);

private:
    static inline const QUrl ExtractionStartEndpoint{"http://localhost:8080/api/extraction/start"};
    static inline const QUrl ExtractionSectionsEndpoint{"http://localhost:8080/api/extraction/sections"};
    static inline const QUrl ExtractionAcceptEndpoint{"http://localhost:8080/api/extraction/accept"};

    QNetworkAccessManager _network;
    Logger _log{"QmlClient.ExtractionClient"};
};

#endif // EXTRACTION_CLIENT_H
