#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QTimer>

class ModelDownloader;

class TrayManager : public QObject {
    Q_OBJECT

public:
    explicit TrayManager(QObject* parent = nullptr);
    ~TrayManager() override;

public slots:
    void updateStatus();
    void startBackend();
    void stopBackend();
    void restartBackend();
    void openClient();
    void stopAndQuit();
    void quitTrayOnly();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onDownloadModelsTriggered();
    void onModelDownloadCompleted();
    void onModelDownloadError();
    void onModelDownloadProgress();

private:
    bool isBackendRunning() const;
    bool isServiceMode() const;
    void ensureBackendStarted();

    QSystemTrayIcon* m_trayIcon{nullptr};
    QMenu* m_trayMenu{nullptr};
    QAction* m_openClientAction{nullptr};
    QAction* m_statusAction{nullptr};
    QAction* m_startAction{nullptr};
    QAction* m_stopAction{nullptr};
    QAction* m_restartAction{nullptr};
    QAction* m_downloadModelsAction{nullptr};
    QAction* m_stopAndQuitAction{nullptr};
    QAction* m_quitTrayOnlyAction{nullptr};
    QTimer* m_pollTimer{nullptr};
    ModelDownloader* m_modelDownloader{nullptr};
};
