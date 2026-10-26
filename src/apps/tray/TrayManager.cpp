#include "TrayManager.h"
#include "constants/CommonConstants.h"
#include "model/ModelDownloader.h"
#include "system/WindowsService.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QIcon>
#include <QProcess>
#include <QStyle>
#include <QThread>

#ifdef _WIN32
#include <windows.h>
#endif

TrayManager::TrayManager(QObject* parent)
    : QObject(parent)
{
    m_trayIcon = new QSystemTrayIcon(this);

    // Attempt to load application icon from app directory or fallback to system icon
    QString iconPath = QCoreApplication::applicationDirPath() + "/" + AppInfo::IconFileName;
    QIcon icon(iconPath);
    if (icon.isNull()) {
        icon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    }
    m_trayIcon->setIcon(icon);

    m_trayMenu = new QMenu();

    m_openClientAction = m_trayMenu->addAction(QString("Open %1").arg(AppInfo::ClientAppName));
    connect(m_openClientAction, &QAction::triggered, this, &TrayManager::openClient);

    m_trayMenu->addSeparator();

    m_statusAction = m_trayMenu->addAction(QString("%1 Status: Checking...").arg(AppInfo::ServerAppName));
    m_statusAction->setEnabled(false);

    m_startAction = m_trayMenu->addAction("Start Engine");
    connect(m_startAction, &QAction::triggered, this, &TrayManager::startBackend);

    m_stopAction = m_trayMenu->addAction("Stop Engine");
    connect(m_stopAction, &QAction::triggered, this, &TrayManager::stopBackend);

    m_restartAction = m_trayMenu->addAction("Restart Engine");
    connect(m_restartAction, &QAction::triggered, this, &TrayManager::restartBackend);

    m_trayMenu->addSeparator();

    m_downloadModelsAction = m_trayMenu->addAction("Download AI Models...");
    connect(m_downloadModelsAction, &QAction::triggered, this, &TrayManager::onDownloadModelsTriggered);

    m_trayMenu->addSeparator();

    m_exitAction = m_trayMenu->addAction("Exit");
    connect(m_exitAction, &QAction::triggered, this, &TrayManager::exitApplication);

    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &TrayManager::onTrayActivated);
    connect(qApp, &QCoreApplication::aboutToQuit, this, &TrayManager::onAboutToQuit);

    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, &TrayManager::updateStatus);
    m_pollTimer->start(2500);

    m_modelDownloader = new ModelDownloader(this);
    connect(m_modelDownloader, &ModelDownloader::downloadCompleted, this, &TrayManager::onModelDownloadCompleted);
    connect(m_modelDownloader, &ModelDownloader::errorChanged, this, &TrayManager::onModelDownloadError);
    connect(m_modelDownloader, &ModelDownloader::progressChanged, this, &TrayManager::onModelDownloadProgress);
    connect(m_modelDownloader, &ModelDownloader::modelsReadyChanged, this, &TrayManager::updateStatus);

    updateStatus();
    m_trayIcon->show();

    // Auto-start backend application if not already running
    ensureBackendStarted();

    // Auto-launch client UI
    openClient();
}

TrayManager::~TrayManager()
{
    delete m_trayMenu;
}

bool TrayManager::isBackendRunning() const
{
    if (WindowsService::isServiceRunning()) {
        return true;
    }
#ifdef _WIN32
    std::wstring globalMutex = (L"Global\\" + QString(AppInfo::DaemonMutexName)).toStdWString();
    HANDLE hGlobal = OpenMutexW(SYNCHRONIZE, FALSE, globalMutex.c_str());
    if (hGlobal) {
        CloseHandle(hGlobal);
        return true;
    } else if (GetLastError() == ERROR_ACCESS_DENIED) {
        return true;
    }

    std::wstring localMutex = (L"Local\\" + QString(AppInfo::DaemonMutexName)).toStdWString();
    HANDLE hLocal = OpenMutexW(SYNCHRONIZE, FALSE, localMutex.c_str());
    if (hLocal) {
        CloseHandle(hLocal);
        return true;
    } else if (GetLastError() == ERROR_ACCESS_DENIED) {
        return true;
    }
#endif
    return false;
}

bool TrayManager::isServiceMode() const
{
    return WindowsService::isServiceInstalled();
}

void TrayManager::ensureBackendStarted()
{
    if (!isBackendRunning()) {
        startBackend();
    }
}

void TrayManager::updateStatus()
{
    bool svcRunning = WindowsService::isServiceRunning();
    bool procRunning = !svcRunning && isBackendRunning();
    bool anyRunning = svcRunning || procRunning;
    QString workloadStr = QString::fromUtf8(AppInfo::WorkloadTitle);

    if (svcRunning) {
        m_statusAction->setText(QString("%1: Running (Service • %2)").arg(AppInfo::ServerAppName, workloadStr));
        m_trayIcon->setToolTip(QString("%1 (%2)\n%3: Running (Windows Service)").arg(AppInfo::WorkspaceAppName, workloadStr, AppInfo::ServerAppName));
    } else if (procRunning) {
        m_statusAction->setText(QString("%1: Running (Process • %2)").arg(AppInfo::ServerAppName, workloadStr));
        m_trayIcon->setToolTip(QString("%1 (%2)\n%3: Running (Background Process)").arg(AppInfo::WorkspaceAppName, workloadStr, AppInfo::ServerAppName));
    } else {
        m_statusAction->setText(QString("%1: Stopped (%2)").arg(AppInfo::ServerAppName, workloadStr));
        m_trayIcon->setToolTip(QString("%1 (%2)\n%3: Stopped").arg(AppInfo::WorkspaceAppName, workloadStr, AppInfo::ServerAppName));
    }

    m_startAction->setEnabled(!anyRunning);
    m_stopAction->setEnabled(anyRunning);
    m_restartAction->setEnabled(anyRunning);

    if (m_downloadModelsAction && m_modelDownloader) {
        if (m_modelDownloader->isDownloading()) {
            m_downloadModelsAction->setText(QString("Downloading Models (%1% • %2)")
                                                .arg(m_modelDownloader->progressPercent())
                                                .arg(m_modelDownloader->downloadSpeed()));
            m_downloadModelsAction->setEnabled(false);
        } else if (m_modelDownloader->areModelsPresent()) {
            m_downloadModelsAction->setText("AI Models: Installed");
            m_downloadModelsAction->setEnabled(true);
        } else {
            m_downloadModelsAction->setText(QString("Download AI Models (%1)...").arg(ModelDownloader::formattedTotalSize()));
            m_downloadModelsAction->setEnabled(true);
        }
    }
}

void TrayManager::startBackend()
{
    if (isBackendRunning()) {
        m_trayIcon->showMessage(
            AppInfo::ServerAppName,
            "Engine is already running.",
            QSystemTrayIcon::Information,
            3000
        );
        updateStatus();
        return;
    }

    if (isServiceMode()) {
        QString msg;
        bool ok = WindowsService::startService(msg);
        m_trayIcon->showMessage(
            AppInfo::ServerAppName,
            msg,
            ok ? QSystemTrayIcon::Information : QSystemTrayIcon::Warning,
            4000
        );
    } else {
        QString appDir = QCoreApplication::applicationDirPath();
        QString serverPath = appDir + "/" + AppInfo::ServerExecutableName;
        if (QFile::exists(serverPath)) {
            bool ok = QProcess::startDetached(serverPath, {}, appDir);
            m_trayIcon->showMessage(
                AppInfo::ServerAppName,
                ok ? "Started background engine process." : QString("Failed to launch %1.").arg(AppInfo::ServerExecutableName),
                ok ? QSystemTrayIcon::Information : QSystemTrayIcon::Warning,
                4000
            );
        } else {
            m_trayIcon->showMessage(
                AppInfo::ServerAppName,
                QString("Executable %1 not found in application directory.").arg(AppInfo::ServerExecutableName),
                QSystemTrayIcon::Warning,
                4000
            );
        }
    }
    updateStatus();
}

void TrayManager::stopBackend()
{
    if (WindowsService::isServiceRunning()) {
        QString msg;
        bool ok = WindowsService::stopService(msg);
        m_trayIcon->showMessage(
            AppInfo::ServerAppName,
            msg,
            ok ? QSystemTrayIcon::Information : QSystemTrayIcon::Warning,
            4000
        );
    } else if (isBackendRunning()) {
#ifdef _WIN32
        QProcess softKill;
        softKill.start("taskkill", {"/IM", AppInfo::ServerExecutableName});
        softKill.waitForFinished(1000);
        if (isBackendRunning()) {
            QProcess forceKill;
            forceKill.start("taskkill", {"/F", "/IM", AppInfo::ServerExecutableName, "/T"});
            forceKill.waitForFinished(2000);
        }
#endif
        m_trayIcon->showMessage(
            AppInfo::ServerAppName,
            "Background engine process stopped.",
            QSystemTrayIcon::Information,
            3000
        );
    } else {
        m_trayIcon->showMessage(
            AppInfo::ServerAppName,
            "Engine is not currently running.",
            QSystemTrayIcon::Information,
            3000
        );
    }
    updateStatus();
}

void TrayManager::restartBackend()
{
    stopBackend();
    QThread::msleep(800);
    startBackend();
}

void TrayManager::exitApplication()
{
    onAboutToQuit();
    qApp->quit();
}

void TrayManager::onAboutToQuit()
{
    static bool s_alreadyExiting = false;
    if (s_alreadyExiting) {
        return;
    }
    s_alreadyExiting = true;

#ifdef _WIN32
    // Send graceful close request to client UI first
    QProcess softKillClient;
    softKillClient.start("taskkill", {"/IM", AppInfo::ClientExecutableName});
    softKillClient.waitForFinished(800);
    QProcess forceKillClient;
    forceKillClient.start("taskkill", {"/F", "/IM", AppInfo::ClientExecutableName, "/T"});
    forceKillClient.waitForFinished(800);
#endif
    if (isBackendRunning()) {
        stopBackend();
    }
}

void TrayManager::openClient()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString clientPath = appDir + "/" + AppInfo::ClientExecutableName;
    if (QFile::exists(clientPath)) {
        QProcess::startDetached(clientPath, {}, appDir);
    } else {
        m_trayIcon->showMessage(
            AppInfo::ClientAppName,
            QString("Desktop UI executable (%1) not found.").arg(AppInfo::ClientExecutableName),
            QSystemTrayIcon::Warning,
            3000
        );
    }
}

void TrayManager::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        openClient();
    }
}

void TrayManager::onDownloadModelsTriggered()
{
    if (!m_modelDownloader) {
        return;
    }

    if (m_modelDownloader->areModelsPresent()) {
        m_trayIcon->showMessage(
            AppInfo::WorkspaceAppName,
            "All required AI models are already downloaded and present in local storage.",
            QSystemTrayIcon::Information,
            3000
        );
        return;
    }

    if (!m_modelDownloader->isDownloading()) {
        m_modelDownloader->startDownload();
        m_trayIcon->showMessage(
            AppInfo::WorkspaceAppName,
            QString("Starting background download of AI models (%1)...").arg(ModelDownloader::formattedTotalSize()),
            QSystemTrayIcon::Information,
            3000
        );
    }
    openClient();
}

void TrayManager::onModelDownloadProgress()
{
    if (m_downloadModelsAction && m_modelDownloader && m_modelDownloader->isDownloading()) {
        m_downloadModelsAction->setText(QString("Downloading Models (%1% • %2)")
                                            .arg(m_modelDownloader->progressPercent())
                                            .arg(m_modelDownloader->downloadSpeed()));
    }
}

void TrayManager::onModelDownloadCompleted()
{
    m_trayIcon->showMessage(
        AppInfo::WorkspaceAppName,
        "AI models downloaded successfully! Offline extraction engine is now active.",
        QSystemTrayIcon::Information,
        5000
    );
    updateStatus();
}

void TrayManager::onModelDownloadError()
{
    if (m_modelDownloader && m_modelDownloader->hasError()) {
        m_trayIcon->showMessage(
            AppInfo::WorkspaceAppName,
            m_modelDownloader->errorMessage(),
            QSystemTrayIcon::Warning,
            6000
        );
        updateStatus();
    }
}
