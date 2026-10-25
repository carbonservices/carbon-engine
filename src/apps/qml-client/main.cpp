#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QtWebEngineQuick>

#include "constants/CommonConstants.h"

int main(int argc, char *argv[])
{
    QtWebEngineQuick::initialize();

    QGuiApplication app(argc, argv);
    app.setApplicationName(AppInfo::ClientAppName);
    app.setOrganizationName(AppInfo::OrganizationName);
    app.setOrganizationDomain(AppInfo::OrganizationDomain);
    app.setApplicationDisplayName(AppInfo::ClientAppName);
    app.setApplicationVersion(AppInfo::Version);

    QQuickStyle::setStyle("Basic");

    QIcon appIcon(":/CarbonEngineClient/logo.png");
    if (appIcon.isNull())
    {
        appIcon = QIcon(":/qt/qml/CarbonEngineClient/logo.png");
    }
    if (appIcon.isNull())
    {
        appIcon = QIcon("logo.png");
    }
    app.setWindowIcon(appIcon);

    QQmlApplicationEngine engine;
    engine.loadFromModule("CarbonEngineClient", "Main");

    if (engine.rootObjects().isEmpty())
    {
        return -1;
    }

    return app.exec();
}
