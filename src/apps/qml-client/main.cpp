#include <QFile>
#include <QGuiApplication>
#include <QProcess>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QtQml>
#include <QtWebEngineQuick>

#include "constants/CommonConstants.h"
#include "logging/Logger.h"
#include "model/ModelDownloader.h"
#include "system/SingleInstanceGuard.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

static void qtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
    Logger log(AppInfo::ClientAppName);
    switch (type) {
    case QtDebugMsg:
        break;
    case QtInfoMsg:
        log.info(msg);
        break;
    case QtWarningMsg:
        log.warn(msg);
        break;
    case QtCriticalMsg:
    case QtFatalMsg:
        log.critical(msg);
        break;
    }
}

#ifdef _WIN32
#include <string>

struct WindowFinderContext {
    HWND foundHwnd;
};

static BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam)
{
    WindowFinderContext* ctx = reinterpret_cast<WindowFinderContext*>(lParam);
    WCHAR title[256];
    int len = GetWindowTextW(hwnd, title, 256);
    if (len > 0) {
        std::wstring wtitle(title);
        if (wtitle.find(L"Carbon ESG") != std::wstring::npos) {
            ctx->foundHwnd = hwnd;
            return FALSE;
        }
    }
    return TRUE;
}
#endif

int main(int argc, char *argv[])
{
    SingleInstanceGuard guard(AppInfo::ClientMutexName);
    if (!guard.isPrimary())
    {
#ifdef _WIN32
        WindowFinderContext ctx{NULL};
        EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&ctx));
        if (ctx.foundHwnd) {
            ShowWindow(ctx.foundHwnd, SW_RESTORE);
            SetForegroundWindow(ctx.foundHwnd);
        }
#endif
        return 0;
    }

    QtWebEngineQuick::initialize();

    QGuiApplication app(argc, argv);
    qInstallMessageHandler(qtMessageHandler);
    app.setApplicationName(AppInfo::ClientAppName);
    app.setOrganizationName(AppInfo::OrganizationName);
    app.setOrganizationDomain(AppInfo::OrganizationDomain);
    app.setApplicationDisplayName(AppInfo::ClientAppName);
    app.setApplicationVersion(AppInfo::Version);
    Logger log(AppInfo::ClientAppName);

    QQuickStyle::setStyle("Basic");

    // Auto-launch background engine if not already running (ensures client connects cleanly on Linux and direct Windows launch)
    if (!SingleInstanceGuard::isRunning(AppInfo::DaemonMutexName)) {
        QString appDir = QCoreApplication::applicationDirPath();
        QString serverPath = appDir + "/" + AppInfo::ServerExecutableName;
        if (QFile::exists(serverPath)) {
            QProcess::startDetached(serverPath, {}, appDir);
        } else {
            QProcess::startDetached(AppInfo::ServerExecutableName, {}, appDir);
        }
    }

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

    ModelDownloader modelDownloader;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("backendModelDownloader", &modelDownloader);
    engine.loadFromModule("CarbonEngineClient", "Main");

    if (engine.rootObjects().isEmpty())
    {
        log.critical("Failed to load QML root component from module 'CarbonEngineClient'.");
        return -1;
    }

    return app.exec();
}
