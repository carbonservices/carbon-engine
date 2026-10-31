#include "ApiClientBase.h"

ApiClientBase::ApiClientBase(QObject *parent)
    : QObject(parent)
{
}

QNetworkAccessManager *ApiClientBase::sharedNetworkManager()
{
    static QNetworkAccessManager s_manager;
    return &s_manager;
}

QNetworkAccessManager &ApiClientBase::network()
{
    return *sharedNetworkManager();
}

QUrl ApiClientBase::endpoint(const QString &route)
{
    static const QString s_baseUrl = QStringLiteral("http://localhost:%1").arg(ServerDefaults::DefaultPort);

    QString cleanRoute = route.startsWith('/') ? route : QString("/" + route);
    return QUrl(s_baseUrl + cleanRoute);
}

QNetworkRequest ApiClientBase::createJsonRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return request;
}
