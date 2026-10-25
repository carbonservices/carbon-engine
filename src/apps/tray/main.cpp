#include "TrayManager.h"
#include "constants/CommonConstants.h"
#include "system/SingleInstanceGuard.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char* argv[])
{
    SingleInstanceGuard guard(AppInfo::TrayMutexName);
    if (!guard.isPrimary()) {
        // Another instance of the tray companion is already active in the taskbar notification area
        return 0;
    }

    QApplication app(argc, argv);
    app.setApplicationName(AppInfo::TrayAppName);
    app.setOrganizationName(AppInfo::OrganizationName);
    app.setOrganizationDomain(AppInfo::OrganizationDomain);
    app.setApplicationVersion(AppInfo::Version);
    app.setQuitOnLastWindowClosed(false);

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, AppInfo::TrayAppName, "System tray notification area is not supported on this platform.");
        return 1;
    }

    TrayManager trayManager;

    return app.exec();
}

