#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QSettings>

class GitHubClient : public QObject
{
    Q_OBJECT

    // Versions & Releases
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY releaseInfoChanged)
    Q_PROPERTY(QString releaseName READ releaseName NOTIFY releaseInfoChanged)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY releaseInfoChanged)
    Q_PROPERTY(QString releaseUrl READ releaseUrl NOTIFY releaseInfoChanged)
    Q_PROPERTY(QString installerDownloadUrl READ installerDownloadUrl NOTIFY releaseInfoChanged)
    Q_PROPERTY(qint64 installerFileSize READ installerFileSize NOTIFY releaseInfoChanged)
    Q_PROPERTY(QString installerFileName READ installerFileName NOTIFY releaseInfoChanged)
    Q_PROPERTY(bool isCheckingUpdate READ isCheckingUpdate NOTIFY checkingUpdateChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY releaseInfoChanged)
    Q_PROPERTY(bool shouldPromptUpdate READ shouldPromptUpdate NOTIFY promptStateChanged)

    // Occasional Feedback Prompt
    Q_PROPERTY(bool shouldPromptFeedback READ shouldPromptFeedback NOTIFY promptStateChanged)

    // Issue Submission State
    Q_PROPERTY(bool isSubmittingIssue READ isSubmittingIssue NOTIFY submittingIssueChanged)
    Q_PROPERTY(QString lastIssueUrl READ lastIssueUrl NOTIFY issueSubmitted)
    Q_PROPERTY(int lastIssueNumber READ lastIssueNumber NOTIFY issueSubmitted)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)

    // Configuration / Settings
    Q_PROPERTY(QString repoOwner READ repoOwner WRITE setRepoOwner NOTIFY settingsChanged)
    Q_PROPERTY(QString repoName READ repoName WRITE setRepoName NOTIFY settingsChanged)
    Q_PROPERTY(QString githubToken READ githubToken WRITE setGithubToken NOTIFY settingsChanged)
    Q_PROPERTY(bool hasGithubToken READ hasGithubToken NOTIFY settingsChanged)
    Q_PROPERTY(QString cloudProxyUrl READ cloudProxyUrl WRITE setCloudProxyUrl NOTIFY settingsChanged)
    Q_PROPERTY(bool useCloudProxy READ useCloudProxy WRITE setUseCloudProxy NOTIFY settingsChanged)
    Q_PROPERTY(bool autoCheckUpdates READ autoCheckUpdates WRITE setAutoCheckUpdates NOTIFY settingsChanged)

public:
    explicit GitHubClient(QObject *parent = nullptr);
    ~GitHubClient() override;

    QString currentVersion() const;
    QString latestVersion() const { return m_latestVersion; }
    QString releaseName() const { return m_releaseName; }
    QString releaseNotes() const { return m_releaseNotes; }
    QString releaseUrl() const { return m_releaseUrl; }
    QString installerDownloadUrl() const { return m_installerDownloadUrl; }
    qint64 installerFileSize() const { return m_installerFileSize; }
    QString installerFileName() const { return m_installerFileName; }
    bool isCheckingUpdate() const { return m_isCheckingUpdate; }
    bool updateAvailable() const { return m_updateAvailable; }
    bool shouldPromptUpdate() const { return m_shouldPromptUpdate; }

    bool shouldPromptFeedback() const { return m_shouldPromptFeedback; }

    bool isSubmittingIssue() const { return m_isSubmittingIssue; }
    QString lastIssueUrl() const { return m_lastIssueUrl; }
    int lastIssueNumber() const { return m_lastIssueNumber; }
    QString statusMessage() const { return m_statusMessage; }
    QString errorMessage() const { return m_errorMessage; }
    bool hasError() const { return m_hasError; }

    QString repoOwner() const { return m_repoOwner; }
    void setRepoOwner(const QString &owner);

    QString repoName() const { return m_repoName; }
    void setRepoName(const QString &repo);

    QString githubToken() const { return m_githubToken; }
    void setGithubToken(const QString &token);
    bool hasGithubToken() const { return !m_githubToken.trimmed().isEmpty(); }

    QString cloudProxyUrl() const { return m_cloudProxyUrl; }
    void setCloudProxyUrl(const QString &url);

    bool useCloudProxy() const { return m_useCloudProxy; }
    void setUseCloudProxy(bool enabled);

    bool autoCheckUpdates() const { return m_autoCheckUpdates; }
    void setAutoCheckUpdates(bool enabled);

    // QML Invocable APIs
    Q_INVOKABLE void checkForUpdates(bool silent = false);
    Q_INVOKABLE void dismissCurrentUpdate();
    Q_INVOKABLE void openReleaseInBrowser();
    Q_INVOKABLE void downloadAndInstallUpdate();

    Q_INVOKABLE void submitBugReport(
        const QString &title,
        const QString &description,
        const QString &steps,
        const QString &severity,
        bool includeLogs = true,
        bool includeSysInfo = true
    );

    Q_INVOKABLE void openBugReportInBrowser(
        const QString &title,
        const QString &description,
        const QString &steps,
        const QString &severity,
        bool includeLogs = true,
        bool includeSysInfo = true
    );

    Q_INVOKABLE void submitFeedback(
        int rating,
        const QString &category,
        const QString &comments,
        bool includeSysInfo = true
    );

    Q_INVOKABLE void openFeedbackInBrowser(
        int rating,
        const QString &category,
        const QString &comments,
        bool includeSysInfo = true
    );

    Q_INVOKABLE void snoozeFeedback(int days = 7);
    Q_INVOKABLE void dismissFeedbackForever();
    Q_INVOKABLE void recordReportCompleted();
    Q_INVOKABLE void checkFeedbackPromptEligibility();

    Q_INVOKABLE QString getDiagnosticPreview(bool includeLogs = true, bool includeSysInfo = true);
    Q_INVOKABLE void copyToClipboard(const QString &text);
    Q_INVOKABLE void clearStatus();

    Q_INVOKABLE void saveSettings(
        const QString &token,
        const QString &owner,
        const QString &repo,
        bool autoCheck,
        const QString &proxyUrl = QString(),
        bool useProxy = true
    );
    Q_INVOKABLE void resetSettings();

    static bool isNewerVersion(const QString &currentVer, const QString &remoteVer);

signals:
    void checkingUpdateChanged();
    void releaseInfoChanged();
    void submittingIssueChanged();
    void issueSubmitted(int issueNumber, const QString &issueUrl);
    void issueSubmissionFailed(const QString &reason);
    void statusChanged();
    void errorChanged();
    void settingsChanged();
    void promptStateChanged();
    void updatePromptTriggered(const QString &version, const QString &url);
    void feedbackPromptTriggered();

private slots:
    void onReleaseCheckFinished();
    void onIssuePostFinished();
    void onCloudIssuePostFinished();

private:
    void loadSettings();
    void createGitHubIssue(const QString &title, const QString &body, const QStringList &labels);
    void createCloudIssue(const QString &endpointPath, const QJsonObject &payload);
    QUrl buildNewIssueWebUrl(const QString &title, const QString &body, const QStringList &labels) const;
    void setStatus(const QString &msg);
    void setError(const QString &err);

    QNetworkAccessManager *m_nam = nullptr;

    QString m_repoOwner;
    QString m_repoName;
    QString m_githubToken;
    QString m_cloudProxyUrl;
    bool m_useCloudProxy = true;
    bool m_autoCheckUpdates = true;

    // Releases State
    bool m_isCheckingUpdate = false;
    bool m_updateAvailable = false;
    bool m_shouldPromptUpdate = false;
    QString m_latestVersion;
    QString m_releaseName;
    QString m_releaseNotes;
    QString m_releaseUrl;
    QString m_installerDownloadUrl;
    qint64 m_installerFileSize = 0;
    QString m_installerFileName;

    // Feedback State
    bool m_shouldPromptFeedback = false;

    // Issue Posting State
    bool m_isSubmittingIssue = false;
    QString m_lastIssueUrl;
    int m_lastIssueNumber = 0;
    QString m_statusMessage;
    QString m_errorMessage;
    bool m_hasError = false;

    // Startup timer for non-blocking update check
    QTimer m_startupTimer;
};
