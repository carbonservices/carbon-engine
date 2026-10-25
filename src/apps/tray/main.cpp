#include "TrayManager.h"
#include "constants/CommonConstants.h"
#include "system/SingleInstanceGuard.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QMessageBox>
#include <QProcess>

int main(int argc, char* argv[])
{
    SingleInstanceGuard guard(AppInfo::TrayMutexName);
    if (!guard.isPrimary()) {
        // Another instance of the tray companion is already active.
        // Clicking the Start Menu shortcut again opens the client UI.
        QCoreApplication tempApp(argc, argv);
        QString appDir = QCoreApplication::applicationDirPath();
        QString clientPath = appDir + "/" + AppInfo::ClientExecutableName;
        if (QFile::exists(clientPath)) {
            QProcess::startDetached(clientPath, {}, appDir);
        }
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

