#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QtWebEngineQuick>

int main(int argc, char *argv[])
{
    QtWebEngineQuick::initialize();

    QGuiApplication app(argc, argv);
    app.setApplicationName("Carbon Engine");
    app.setApplicationDisplayName("Carbon Engine - BRSR ESG Analytics");

    QIcon appIcon(":/qt/qml/CarbonEngineClient/logo.png");
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