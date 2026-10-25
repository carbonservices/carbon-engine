#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QtWebEngineQuick>

#include "constants/CommonConstants.h"
#include "logging/Logger.h"
#include "system/SingleInstanceGuard.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
    SingleInstanceGuard guard(AppInfo::ClientMutexName);
    if (!guard.isPrimary())
    {
#ifdef _WIN32
        HWND hwnd = FindWindowW(NULL, L"Carbon ESG (Non-GPU Workload)");
        if (!hwnd) {
            hwnd = FindWindowW(NULL, L"Carbon ESG (GPU Accelerated)");
        }
        if (!hwnd) {
            hwnd = FindWindowW(NULL, L"Carbon ESG");
        }
        if (hwnd) {
            ShowWindow(hwnd, SW_RESTORE);
            SetForegroundWindow(hwnd);
        }
#endif
        return 0;
    }

    QtWebEngineQuick::initialize();

    QGuiApplication app(argc, argv);
    app.setApplicationName(AppInfo::ClientAppName);
    app.setOrganizationName(AppInfo::OrganizationName);
    app.setOrganizationDomain(AppInfo::OrganizationDomain);
    app.setApplicationDisplayName(AppInfo::ClientAppName);
    app.setApplicationVersion(AppInfo::Version);
    Logger log(AppInfo::ClientAppName);

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
        log.critical("Failed to load QML root component from module 'CarbonEngineClient'.");
        return -1;
    }

    return app.exec();
}
