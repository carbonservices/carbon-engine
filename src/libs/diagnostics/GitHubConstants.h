#pragma once

#include <QString>

namespace GitHubConstants {
    inline const QString DefaultOwner = QStringLiteral("carbonservices");
    inline const QString DefaultRepo = QStringLiteral("carbon-engine");
    inline const QString ApiBaseUrl = QStringLiteral("https://api.github.com");
    inline const QString ReleasesLatestPath = QStringLiteral("/repos/%1/%2/releases/latest");
    inline const QString IssuesPath = QStringLiteral("/repos/%1/%2/issues");
    inline const QString WebNewIssueUrl = QStringLiteral("https://github.com/%1/%2/issues/new");
    inline const QString WebReleasesUrl = QStringLiteral("https://github.com/%1/%2/releases");

    // Cloud Diagnostics Proxy (Carbon Calculator / Web Service)
    inline const QString DefaultCloudProxyUrl = QStringLiteral("https://carbonesg.online");
    inline const QString CloudBugReportPath = QStringLiteral("/api/bug-report");
    inline const QString CloudFeedbackPath = QStringLiteral("/api/feedback");
    inline const QString KeyCloudProxyUrl = QStringLiteral("diagnostics/cloud_proxy_url");
    inline const QString KeyUseCloudProxy = QStringLiteral("diagnostics/use_cloud_proxy");

    // QSettings Keys
    inline const QString KeyToken = QStringLiteral("github/token");
    inline const QString KeyOwner = QStringLiteral("github/repo_owner");
    inline const QString KeyRepo = QStringLiteral("github/repo_name");
    inline const QString KeyAutoCheck = QStringLiteral("updates/auto_check");
    inline const QString KeyLastCheck = QStringLiteral("updates/last_check");
    inline const QString KeyDismissedVersion = QStringLiteral("updates/dismissed_version");
    inline const QString KeyFeedbackLastPrompt = QStringLiteral("feedback/last_prompt");
    inline const QString KeyFeedbackPromptCount = QStringLiteral("feedback/prompt_count");
    inline const QString KeyFeedbackReportsCount = QStringLiteral("feedback/reports_count");
    inline const QString KeyFeedbackNeverAsk = QStringLiteral("feedback/never_ask");
    inline const QString KeyFeedbackSubmitted = QStringLiteral("feedback/has_submitted");
    inline const QString KeyLaunchCount = QStringLiteral("app/launch_count");
    inline const QString KeyFirstLaunchTime = QStringLiteral("app/first_launch_time");

    // Labels
    inline const QString LabelBug = QStringLiteral("bug");
    inline const QString LabelFeedback = QStringLiteral("feedback");
    inline const QString LabelRating = QStringLiteral("rating");
    inline const QString LabelUserReport = QStringLiteral("user-report");
}
