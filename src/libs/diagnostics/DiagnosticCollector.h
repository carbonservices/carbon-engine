#pragma once

#include <QString>
#include <QJsonObject>

class DiagnosticCollector {
public:
    static QString collectSystemInfoMarkdown();
    static QString collectModelInfoMarkdown();
    static QString collectLogsMarkdown(int maxLines = 80);

    static QString buildBugReportMarkdown(
        const QString &description,
        const QString &stepsToReproduce,
        const QString &severity,
        bool includeLogs,
        bool includeSysInfo
    );

    static QString buildFeedbackMarkdown(
        int rating,
        const QString &category,
        const QString &comments,
        bool includeSysInfo
    );

    static QJsonObject collectDiagnosticsJson(bool includeLogs = true);

private:
    static QString sanitizeLogContent(const QString &raw);
    static QString formatBytes(qint64 bytes);
};

