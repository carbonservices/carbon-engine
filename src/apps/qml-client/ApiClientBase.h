#ifndef API_CLIENT_BASE_H
#define API_CLIENT_BASE_H

#include "logging/Logger.h"
#include "constants/CommonConstants.h"

#include <QObject>
#include <QQmlEngine>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include <QString>

class ApiClientBase : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    explicit ApiClientBase(QObject *parent = nullptr);
    ~ApiClientBase() override = default;

    static QNetworkAccessManager *sharedNetworkManager();
    static QUrl endpoint(const QString &route);
    static QNetworkRequest createJsonRequest(const QUrl &url);

    Q_INVOKABLE QString baseUrl() const { return endpoint(QStringLiteral("/")).toString(); }
    Q_INVOKABLE QUrl serverEndpoint(const QString &route = QStringLiteral("/")) const { return endpoint(route); }

protected:
    QNetworkAccessManager &network();
};

#endif // API_CLIENT_BASE_H

