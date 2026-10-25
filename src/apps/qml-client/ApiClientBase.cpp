#include "ApiClientBase.h"

#include <QProcessEnvironment>

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
    static const QString s_baseUrl = []() {
        QString envUrl = QProcessEnvironment::systemEnvironment().value("CARBON_ENGINE_SERVER_URL").trimmed();
        if (!envUrl.isEmpty()) {
            if (envUrl.endsWith('/')) {
                envUrl.chop(1);
            }
            return envUrl;
        }
        return QStringLiteral("http://localhost:%1").arg(ServerDefaults::DefaultPort);
    }();

    QString cleanRoute = route.startsWith('/') ? route : QString("/" + route);
    return QUrl(s_baseUrl + cleanRoute);
}

QNetworkRequest ApiClientBase::createJsonRequest(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return request;
}

