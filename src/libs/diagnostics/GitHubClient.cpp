#include "GitHubClient.h"
#include "GitHubConstants.h"
#include "DiagnosticCollector.h"
#include "constants/CommonConstants.h"
#include "logging/Logger.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDesktopServices>
#include <QClipboard>
#include <QGuiApplication>
#include <QRegularExpression>

static Logger s_log(QStringLiteral("Diagnostics.GitHubClient"));

GitHubClient::GitHubClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    loadSettings();

    // Track launch count and first launch time for feedback heuristics
    QSettings settings;
    int launches = settings.value(GitHubConstants::KeyLaunchCount, 0).toInt() + 1;
    settings.setValue(GitHubConstants::KeyLaunchCount, launches);

    if (!settings.contains(GitHubConstants::KeyFirstLaunchTime)) {
        settings.setValue(GitHubConstants::KeyFirstLaunchTime, QDateTime::currentDateTime());
    }

    // Schedule initial non-blocking update check and feedback prompt check
    connect(&m_startupTimer, &QTimer::timeout, this, [this]() {
        m_startupTimer.stop();
        if (m_autoCheckUpdates) {
            checkForUpdates(true /* silent */);
        }
        checkFeedbackPromptEligibility();
    });
    m_startupTimer.setSingleShot(true);
    m_startupTimer.start(4000); // 4 seconds after launch so app UI renders instantly
}

GitHubClient::~GitHubClient() = default;

QString GitHubClient::currentVersion() const
{
    return QString::fromUtf8(AppInfo::Version);
}

void GitHubClient::loadSettings()
{
    QSettings settings;
    m_repoOwner = settings.value(GitHubConstants::KeyOwner, GitHubConstants::DefaultOwner).toString();
    m_repoName = settings.value(GitHubConstants::KeyRepo, GitHubConstants::DefaultRepo).toString();
    m_githubToken = settings.value(GitHubConstants::KeyToken, QString()).toString();
    m_cloudProxyUrl = settings.value(GitHubConstants::KeyCloudProxyUrl, GitHubConstants::DefaultCloudProxyUrl).toString();
    m_useCloudProxy = settings.value(GitHubConstants::KeyUseCloudProxy, true).toBool();
    m_autoCheckUpdates = settings.value(GitHubConstants::KeyAutoCheck, true).toBool();
}

void GitHubClient::setRepoOwner(const QString &owner)
{
    QString clean = owner.trimmed();
    if (clean.isEmpty()) clean = GitHubConstants::DefaultOwner;
    if (m_repoOwner != clean) {
        m_repoOwner = clean;
        QSettings().setValue(GitHubConstants::KeyOwner, m_repoOwner);
        emit settingsChanged();
    }
}

void GitHubClient::setRepoName(const QString &repo)
{
    QString clean = repo.trimmed();
    if (clean.isEmpty()) clean = GitHubConstants::DefaultRepo;
    if (m_repoName != clean) {
        m_repoName = clean;
        QSettings().setValue(GitHubConstants::KeyRepo, m_repoName);
        emit settingsChanged();
    }
}

void GitHubClient::setGithubToken(const QString &token)
{
    QString clean = token.trimmed();
    if (m_githubToken != clean) {
        m_githubToken = clean;
        QSettings().setValue(GitHubConstants::KeyToken, m_githubToken);
        emit settingsChanged();
    }
}

void GitHubClient::setCloudProxyUrl(const QString &url)
{
    QString clean = url.trimmed();
    if (clean.isEmpty()) clean = GitHubConstants::DefaultCloudProxyUrl;
    if (m_cloudProxyUrl != clean) {
        m_cloudProxyUrl = clean;
        QSettings().setValue(GitHubConstants::KeyCloudProxyUrl, m_cloudProxyUrl);
        emit settingsChanged();
    }
}

void GitHubClient::setUseCloudProxy(bool enabled)
{
    if (m_useCloudProxy != enabled) {
        m_useCloudProxy = enabled;
        QSettings().setValue(GitHubConstants::KeyUseCloudProxy, m_useCloudProxy);
        emit settingsChanged();
    }
}

void GitHubClient::setAutoCheckUpdates(bool enabled)
{
    if (m_autoCheckUpdates != enabled) {
        m_autoCheckUpdates = enabled;
        QSettings().setValue(GitHubConstants::KeyAutoCheck, m_autoCheckUpdates);
        emit settingsChanged();
    }
}

void GitHubClient::saveSettings(
    const QString &token,
    const QString &owner,
    const QString &repo,
    bool autoCheck,
    const QString &proxyUrl,
    bool useProxy)
{
    setGithubToken(token);
    setRepoOwner(owner);
    setRepoName(repo);
    setAutoCheckUpdates(autoCheck);
    if (!proxyUrl.trimmed().isEmpty()) {
        setCloudProxyUrl(proxyUrl);
    }
    setUseCloudProxy(useProxy);

    QSettings settings;
    settings.setValue(GitHubConstants::KeyToken, m_githubToken);
    settings.setValue(GitHubConstants::KeyOwner, m_repoOwner);
    settings.setValue(GitHubConstants::KeyRepo, m_repoName);
    settings.setValue(GitHubConstants::KeyAutoCheck, m_autoCheckUpdates);
    settings.setValue(GitHubConstants::KeyCloudProxyUrl, m_cloudProxyUrl);
    settings.setValue(GitHubConstants::KeyUseCloudProxy, m_useCloudProxy);

    setStatus(QStringLiteral("Settings saved successfully."));
    emit settingsChanged();
}

void GitHubClient::resetSettings()
{
    setRepoOwner(GitHubConstants::DefaultOwner);
    setRepoName(GitHubConstants::DefaultRepo);
    setGithubToken(QString());
    setCloudProxyUrl(GitHubConstants::DefaultCloudProxyUrl);
    setUseCloudProxy(true);
    setAutoCheckUpdates(true);

    QSettings settings;
    settings.remove(GitHubConstants::KeyToken);
    settings.remove(GitHubConstants::KeyOwner);
    settings.remove(GitHubConstants::KeyRepo);
    settings.remove(GitHubConstants::KeyAutoCheck);
    settings.remove(GitHubConstants::KeyCloudProxyUrl);
    settings.remove(GitHubConstants::KeyUseCloudProxy);

    setStatus(QStringLiteral("Settings reset to defaults."));
    emit settingsChanged();
}

void GitHubClient::setStatus(const QString &msg)
{
    m_statusMessage = msg;
    m_hasError = false;
    m_errorMessage.clear();
    emit statusChanged();
    emit errorChanged();
}

void GitHubClient::setError(const QString &err)
{
    m_errorMessage = err;
    m_hasError = true;
    s_log.error(err);
    emit errorChanged();
}

void GitHubClient::clearStatus()
{
    m_statusMessage.clear();
    m_errorMessage.clear();
    m_hasError = false;
    emit statusChanged();
    emit errorChanged();
}

bool GitHubClient::isNewerVersion(const QString &currentVer, const QString &remoteVer)
{
    static const QRegularExpression stripRegex(QStringLiteral("^[^0-9]*"));
    QString cClean = currentVer;
    cClean.remove(stripRegex);
    QString rClean = remoteVer;
    rClean.remove(stripRegex);

    QStringList cParts = cClean.split(QLatin1Char('.'));
    QStringList rParts = rClean.split(QLatin1Char('.'));

    int maxLen = qMax(cParts.size(), rParts.size());
    for (int i = 0; i < maxLen; ++i) {
        int cNum = (i < cParts.size()) ? cParts[i].toInt() : 0;
        int rNum = (i < rParts.size()) ? rParts[i].toInt() : 0;
        if (rNum > cNum) return true;
        if (rNum < cNum) return false;
    }
    return false;
}

void GitHubClient::checkForUpdates(bool silent)
{
    if (m_isCheckingUpdate) return;

    m_isCheckingUpdate = true;
    emit checkingUpdateChanged();
    if (!silent) {
        setStatus(QStringLiteral("Checking GitHub for the latest release..."));
    }

    QString path = GitHubConstants::ReleasesLatestPath.arg(m_repoOwner, m_repoName);
    QUrl url(GitHubConstants::ApiBaseUrl + path);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("CarbonESG-Client/%1").arg(AppInfo::Version));
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");

    if (hasGithubToken()) {
        request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_githubToken).toUtf8());
    }

    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, &GitHubClient::onReleaseCheckFinished);
}

void GitHubClient::onReleaseCheckFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    m_isCheckingUpdate = false;
    emit checkingUpdateChanged();

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray responseData = reply->readAll();

    if (reply->error() != QNetworkReply::NoError || statusCode < 200 || statusCode >= 300) {
        QString errStr;
        QJsonParseError parseErr;
        QJsonDocument errDoc = QJsonDocument::fromJson(responseData, &parseErr);
        if (parseErr.error == QJsonParseError::NoError && errDoc.isObject()) {
            errStr = errDoc.object().value(QStringLiteral("message")).toString();
        }
        if (errStr.isEmpty()) {
            errStr = reply->errorString();
        }

        s_log.warn(QStringLiteral("GitHub update check failed (HTTP %1): %2").arg(statusCode).arg(errStr));
        m_updateAvailable = false;
        m_shouldPromptUpdate = false;
        emit releaseInfoChanged();
        emit promptStateChanged();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (!doc.isObject()) {
        s_log.warn(QStringLiteral("Invalid JSON in GitHub release response"));
        return;
    }

    QJsonObject releaseObj = doc.object();
    QString tagName = releaseObj.value(QStringLiteral("tag_name")).toString();
    m_releaseName = releaseObj.value(QStringLiteral("name")).toString();
    m_releaseNotes = releaseObj.value(QStringLiteral("body")).toString();
    m_releaseUrl = releaseObj.value(QStringLiteral("html_url")).toString();

    // Strip leading 'v' or 'release-'
    QString versionStr = tagName;
    if (versionStr.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        versionStr.remove(0, 1);
    }
    m_latestVersion = versionStr.trimmed();

    // Check for matching installer asset for current OS
    m_installerDownloadUrl.clear();
    m_installerFileSize = 0;
    m_installerFileName.clear();

    QJsonArray assets = releaseObj.value(QStringLiteral("assets")).toArray();
#if defined(Q_OS_WIN)
    const QString targetExt = QStringLiteral(".exe");
#elif defined(Q_OS_MACOS)
    const QString targetExt = QStringLiteral(".dmg");
#else
    const QString targetExt = QStringLiteral(".deb");
#endif

    for (const QJsonValue &assetVal : assets) {
        QJsonObject assetObj = assetVal.toObject();
        QString assetName = assetObj.value(QStringLiteral("name")).toString();
        if (assetName.endsWith(targetExt, Qt::CaseInsensitive)) {
            m_installerDownloadUrl = assetObj.value(QStringLiteral("browser_download_url")).toString();
            m_installerFileSize = static_cast<qint64>(assetObj.value(QStringLiteral("size")).toDouble());
            m_installerFileName = assetName;
            break;
        }
    }

    // Compare with current running version
    QString currVer = currentVersion();
    m_updateAvailable = isNewerVersion(currVer, m_latestVersion);

    // Check if user previously dismissed this version
    QSettings settings;
    settings.setValue(GitHubConstants::KeyLastCheck, QDateTime::currentDateTime());
    QString dismissedVer = settings.value(GitHubConstants::KeyDismissedVersion).toString();

    if (m_updateAvailable && dismissedVer != m_latestVersion) {
        m_shouldPromptUpdate = true;
        emit updatePromptTriggered(m_latestVersion, m_releaseUrl);
    } else {
        m_shouldPromptUpdate = false;
    }

    emit releaseInfoChanged();
    emit promptStateChanged();

    if (m_updateAvailable) {
        setStatus(QStringLiteral("New release v%1 is available!").arg(m_latestVersion));
    } else {
        setStatus(QStringLiteral("Carbon ESG is up to date (v%1).").arg(currVer));
    }
}

void GitHubClient::dismissCurrentUpdate()
{
    m_shouldPromptUpdate = false;
    if (!m_latestVersion.isEmpty()) {
        QSettings settings;
        settings.setValue(GitHubConstants::KeyDismissedVersion, m_latestVersion);
    }
    emit promptStateChanged();
}

void GitHubClient::openReleaseInBrowser()
{
    QUrl url(m_releaseUrl.isEmpty() ? GitHubConstants::WebReleasesUrl.arg(m_repoOwner, m_repoName) : m_releaseUrl);
    QDesktopServices::openUrl(url);
}

void GitHubClient::downloadAndInstallUpdate()
{
    if (m_installerDownloadUrl.isEmpty()) {
        openReleaseInBrowser();
        return;
    }
    QDesktopServices::openUrl(QUrl(m_installerDownloadUrl));
}

void GitHubClient::checkFeedbackPromptEligibility()
{
    QSettings settings;
    if (settings.value(GitHubConstants::KeyFeedbackNeverAsk, false).toBool()) {
        m_shouldPromptFeedback = false;
        emit promptStateChanged();
        return;
    }

    if (settings.value(GitHubConstants::KeyFeedbackSubmitted, false).toBool()) {
        m_shouldPromptFeedback = false;
        emit promptStateChanged();
        return;
    }

    QDateTime lastPrompt = settings.value(GitHubConstants::KeyFeedbackLastPrompt).toDateTime();
    if (lastPrompt.isValid() && lastPrompt.daysTo(QDateTime::currentDateTime()) < 7) {
        m_shouldPromptFeedback = false;
        emit promptStateChanged();
        return;
    }

    int reportsCount = settings.value(GitHubConstants::KeyFeedbackReportsCount, 0).toInt();
    int launchCount = settings.value(GitHubConstants::KeyLaunchCount, 0).toInt();

    // Occasional feedback prompt triggers after:
    // 1 report completed, OR after 3 launches
    if (reportsCount >= 1 || launchCount >= 3) {
        m_shouldPromptFeedback = true;
        settings.setValue(GitHubConstants::KeyFeedbackLastPrompt, QDateTime::currentDateTime());
        emit promptStateChanged();
        emit feedbackPromptTriggered();
    }
}

void GitHubClient::snoozeFeedback(int days)
{
    Q_UNUSED(days);
    m_shouldPromptFeedback = false;
    QSettings settings;
    settings.setValue(GitHubConstants::KeyFeedbackLastPrompt, QDateTime::currentDateTime());
    emit promptStateChanged();
}

void GitHubClient::dismissFeedbackForever()
{
    m_shouldPromptFeedback = false;
    QSettings settings;
    settings.setValue(GitHubConstants::KeyFeedbackNeverAsk, true);
    emit promptStateChanged();
}

void GitHubClient::recordReportCompleted()
{
    QSettings settings;
    int count = settings.value(GitHubConstants::KeyFeedbackReportsCount, 0).toInt() + 1;
    settings.setValue(GitHubConstants::KeyFeedbackReportsCount, count);
    checkFeedbackPromptEligibility();
}

QUrl GitHubClient::buildNewIssueWebUrl(const QString &title, const QString &body, const QStringList &labels) const
{
    QUrl url(GitHubConstants::WebNewIssueUrl.arg(m_repoOwner, m_repoName));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("title"), title);
    query.addQueryItem(QStringLiteral("body"), body);
    if (!labels.isEmpty()) {
        query.addQueryItem(QStringLiteral("labels"), labels.join(QLatin1Char(',')));
    }
    url.setQuery(query);
    return url;
}

void GitHubClient::createGitHubIssue(const QString &title, const QString &body, const QStringList &labels)
{
    m_isSubmittingIssue = true;
    m_lastIssueUrl.clear();
    m_lastIssueNumber = 0;
    emit submittingIssueChanged();
    setStatus(QStringLiteral("Uploading issue to GitHub..."));

    QString path = GitHubConstants::IssuesPath.arg(m_repoOwner, m_repoName);
    QUrl url(GitHubConstants::ApiBaseUrl + path);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("CarbonESG-Client/%1").arg(AppInfo::Version));
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_githubToken).toUtf8());

    QJsonObject payload;
    payload[QStringLiteral("title")] = title;
    payload[QStringLiteral("body")] = body;

    QJsonArray labelsArr;
    for (const QString &label : labels) {
        labelsArr.append(label);
    }
    payload[QStringLiteral("labels")] = labelsArr;

    QNetworkReply *reply = m_nam->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, &GitHubClient::onIssuePostFinished);
}

void GitHubClient::createCloudIssue(const QString &endpointPath, const QJsonObject &payload)
{
    m_isSubmittingIssue = true;
    m_lastIssueUrl.clear();
    m_lastIssueNumber = 0;
    emit submittingIssueChanged();
    setStatus(QStringLiteral("Submitting via Carbon Cloud diagnostics proxy..."));

    QString baseUrl = m_cloudProxyUrl.trimmed();
    if (baseUrl.endsWith(QLatin1Char('/'))) {
        baseUrl.chop(1);
    }
    QUrl url(baseUrl + endpointPath);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("CarbonESG-Client/%1").arg(AppInfo::Version));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_nam->post(request, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, this, &GitHubClient::onCloudIssuePostFinished);
}

void GitHubClient::onIssuePostFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    m_isSubmittingIssue = false;
    emit submittingIssueChanged();

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray responseData = reply->readAll();

    if (reply->error() != QNetworkReply::NoError || statusCode < 200 || statusCode >= 300) {
        QString errStr;
        QJsonParseError parseErr;
        QJsonDocument errDoc = QJsonDocument::fromJson(responseData, &parseErr);
        if (parseErr.error == QJsonParseError::NoError && errDoc.isObject()) {
            errStr = errDoc.object().value(QStringLiteral("message")).toString();
        }
        if (errStr.isEmpty()) {
            errStr = reply->errorString();
        }

        if (statusCode == 401) {
            setError(QStringLiteral("Authentication failed. Please verify your GitHub Personal Access Token in Settings."));
        } else if (statusCode == 403) {
            setError(QStringLiteral("GitHub API rate limit exceeded or token lacks 'issues: write' permission: %1").arg(errStr));
        } else if (statusCode == 404) {
            setError(QStringLiteral("Repository %1/%2 not found or token has insufficient permissions.").arg(m_repoOwner, m_repoName));
        } else {
            setError(QStringLiteral("Failed to create issue: %1 (HTTP %2)").arg(errStr).arg(statusCode));
        }

        emit issueSubmissionFailed(m_errorMessage);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isObject()) {
        QJsonObject resObj = doc.object();
        m_lastIssueNumber = resObj.value(QStringLiteral("number")).toInt();
        m_lastIssueUrl = resObj.value(QStringLiteral("html_url")).toString();

        setStatus(QStringLiteral("Issue #%1 submitted successfully to GitHub!").arg(m_lastIssueNumber));
        s_log.info(QStringLiteral("Issue #%1 created: %2").arg(m_lastIssueNumber).arg(m_lastIssueUrl));
        emit issueSubmitted(m_lastIssueNumber, m_lastIssueUrl);
    }
}

void GitHubClient::onCloudIssuePostFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    m_isSubmittingIssue = false;
    emit submittingIssueChanged();

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray responseData = reply->readAll();

    if (reply->error() != QNetworkReply::NoError || statusCode < 200 || statusCode >= 300) {
        QString errStr;
        QJsonParseError parseErr;
        QJsonDocument errDoc = QJsonDocument::fromJson(responseData, &parseErr);
        if (parseErr.error == QJsonParseError::NoError && errDoc.isObject()) {
            QJsonObject errObj = errDoc.object();
            errStr = errObj.value(QStringLiteral("error")).toString();
        }
        if (errStr.isEmpty()) {
            errStr = reply->errorString();
        }

        setError(QStringLiteral("Cloud submission failed: %1 (HTTP %2)").arg(errStr).arg(statusCode));
        s_log.warn(QStringLiteral("Cloud diagnostics proxy error (%1): %2").arg(statusCode).arg(errStr));
        emit issueSubmissionFailed(m_errorMessage);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isObject()) {
        QJsonObject resObj = doc.object();
        m_lastIssueNumber = resObj.value(QStringLiteral("issue_number")).toInt();
        m_lastIssueUrl = resObj.value(QStringLiteral("issue_url")).toString();

        setStatus(QStringLiteral("Issue #%1 submitted successfully to GitHub!").arg(m_lastIssueNumber));
        s_log.info(QStringLiteral("Issue #%1 created via cloud proxy: %2").arg(m_lastIssueNumber).arg(m_lastIssueUrl));
        emit issueSubmitted(m_lastIssueNumber, m_lastIssueUrl);
    }
}

void GitHubClient::submitBugReport(
    const QString &title,
    const QString &description,
    const QString &steps,
    const QString &severity,
    bool includeLogs,
    bool includeSysInfo)
{
    QString formattedTitle = title.trimmed();
    if (!formattedTitle.startsWith(QStringLiteral("[Bug]"), Qt::CaseInsensitive)) {
        formattedTitle = QStringLiteral("[Bug] 🐞 ") + formattedTitle;
    } else if (!formattedTitle.contains(QStringLiteral("🐞"))) {
        formattedTitle.replace(0, 5, QStringLiteral("[Bug] 🐞 "));
    }

    QString body = DiagnosticCollector::buildBugReportMarkdown(
        description, steps, severity, includeLogs, includeSysInfo);

    QStringList labels = { GitHubConstants::LabelBug, GitHubConstants::LabelUserReport, QStringLiteral("🐞") };
    if (!severity.trimmed().isEmpty()) {
        labels.append(severity.trimmed().toLower());
    }

    if (hasGithubToken()) {
        createGitHubIssue(formattedTitle, body, labels);
    } else if (m_useCloudProxy && !m_cloudProxyUrl.trimmed().isEmpty()) {
        QJsonObject payload;
        payload[QStringLiteral("title")] = formattedTitle;
        payload[QStringLiteral("description")] = description;
        payload[QStringLiteral("steps")] = steps;
        payload[QStringLiteral("severity")] = severity;
        payload[QStringLiteral("version")] = AppInfo::Version;
        payload[QStringLiteral("markdown")] = body;
        payload[QStringLiteral("logs")] = includeLogs ? DiagnosticCollector::collectLogsMarkdown(80) : QString();
        payload[QStringLiteral("sys_info")] = includeSysInfo ? DiagnosticCollector::collectSystemInfoMarkdown() : QString();
        payload[QStringLiteral("model_info")] = includeSysInfo ? DiagnosticCollector::collectModelInfoMarkdown() : QString();
        createCloudIssue(GitHubConstants::CloudBugReportPath, payload);
    } else {
        openBugReportInBrowser(title, description, steps, severity, includeLogs, includeSysInfo);
    }
}

void GitHubClient::openBugReportInBrowser(
    const QString &title,
    const QString &description,
    const QString &steps,
    const QString &severity,
    bool includeLogs,
    bool includeSysInfo)
{
    QString formattedTitle = title.trimmed();
    if (!formattedTitle.startsWith(QStringLiteral("[Bug]"), Qt::CaseInsensitive)) {
        formattedTitle = QStringLiteral("[Bug] 🐞 ") + formattedTitle;
    } else if (!formattedTitle.contains(QStringLiteral("🐞"))) {
        formattedTitle.replace(0, 5, QStringLiteral("[Bug] 🐞 "));
    }

    QString body = DiagnosticCollector::buildBugReportMarkdown(
        description, steps, severity, includeLogs, includeSysInfo);

    QStringList labels = { GitHubConstants::LabelBug, GitHubConstants::LabelUserReport, QStringLiteral("🐞") };
    QUrl url = buildNewIssueWebUrl(formattedTitle, body, labels);
    QDesktopServices::openUrl(url);
    setStatus(QStringLiteral("Opened pre-filled bug report in your web browser."));
}

void GitHubClient::submitFeedback(
    int rating,
    const QString &category,
    const QString &comments,
    bool includeSysInfo)
{
    QString title = QStringLiteral("[Feedback] ⭐ %1/5 - %2").arg(rating).arg(category.trimmed());
    QString body = DiagnosticCollector::buildFeedbackMarkdown(rating, category, comments, includeSysInfo);

    QStringList labels = { GitHubConstants::LabelFeedback, GitHubConstants::LabelRating };

    QSettings settings;
    settings.setValue(GitHubConstants::KeyFeedbackSubmitted, true);
    m_shouldPromptFeedback = false;
    emit promptStateChanged();

    if (hasGithubToken()) {
        createGitHubIssue(title, body, labels);
    } else if (m_useCloudProxy && !m_cloudProxyUrl.trimmed().isEmpty()) {
        QJsonObject payload;
        payload[QStringLiteral("rating")] = rating;
        payload[QStringLiteral("category")] = category;
        payload[QStringLiteral("comments")] = comments;
        payload[QStringLiteral("version")] = AppInfo::Version;
        payload[QStringLiteral("markdown")] = body;
        payload[QStringLiteral("sys_info")] = includeSysInfo ? DiagnosticCollector::collectSystemInfoMarkdown() : QString();
        createCloudIssue(GitHubConstants::CloudFeedbackPath, payload);
    } else {
        openFeedbackInBrowser(rating, category, comments, includeSysInfo);
    }
}

void GitHubClient::openFeedbackInBrowser(
    int rating,
    const QString &category,
    const QString &comments,
    bool includeSysInfo)
{
    QString title = QStringLiteral("[Feedback] ⭐ %1/5 - %2").arg(rating).arg(category.trimmed());
    QString body = DiagnosticCollector::buildFeedbackMarkdown(rating, category, comments, includeSysInfo);

    QStringList labels = { GitHubConstants::LabelFeedback, GitHubConstants::LabelRating };
    QUrl url = buildNewIssueWebUrl(title, body, labels);
    QDesktopServices::openUrl(url);

    QSettings settings;
    settings.setValue(GitHubConstants::KeyFeedbackSubmitted, true);
    m_shouldPromptFeedback = false;
    emit promptStateChanged();
    setStatus(QStringLiteral("Opened pre-filled feedback issue in your web browser."));
}

QString GitHubClient::getDiagnosticPreview(bool includeLogs, bool includeSysInfo)
{
    return DiagnosticCollector::buildBugReportMarkdown(
        QStringLiteral("Sample preview of bug description..."),
        QStringLiteral("1. Open Carbon ESG\n2. Run extraction"),
        QStringLiteral("Normal"),
        includeLogs,
        includeSysInfo
    );
}

void GitHubClient::copyToClipboard(const QString &text)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        clipboard->setText(text);
        setStatus(QStringLiteral("Copied diagnostics to clipboard."));
    }
}
