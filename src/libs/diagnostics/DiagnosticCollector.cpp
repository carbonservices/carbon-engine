#include "DiagnosticCollector.h"
#include "constants/CommonConstants.h"
#include "logging/Logger.h"
#include "model/ModelDownloader.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSysInfo>
#include <QTextStream>
#include <QThread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dxgi.h>
#elif defined(__linux__)
#include <sys/sysinfo.h>
#endif

QString DiagnosticCollector::formatBytes(qint64 bytes)
{
    if (bytes <= 0) return QStringLiteral("0 B");
    const QString units[] = { "B", "KB", "MB", "GB", "TB" };
    double size = bytes;
    int unitIdx = 0;
    while (size >= 1024.0 && unitIdx < 4) {
        size /= 1024.0;
        unitIdx++;
    }
    return QString::asprintf("%.2f %s", size, qPrintable(units[unitIdx]));
}

QString DiagnosticCollector::sanitizeLogContent(const QString &raw)
{
    QString sanitized = raw;
    // Redact GitHub personal access tokens: ghp_, gho_, ghu_, ghs_, ghr_
    static const QRegularExpression ghTokenRegex(QStringLiteral("gh[pousr]_[A-Za-z0-9_]{36,255}"));
    sanitized.replace(ghTokenRegex, QStringLiteral("[REDACTED_GITHUB_TOKEN]"));

    // Redact Bearer tokens in headers
    static const QRegularExpression bearerRegex(QStringLiteral("Bearer\\s+[A-Za-z0-9._\\-~+/]+="));
    sanitized.replace(bearerRegex, QStringLiteral("Bearer [REDACTED]"));

    return sanitized;
}

QString DiagnosticCollector::collectSystemInfoMarkdown()
{
    QString md;
    md += QStringLiteral("| Specification | Detail |\n");
    md += QStringLiteral("| --- | --- |\n");
    md += QStringLiteral("| **Application** | %1 (v%2) |\n").arg(AppInfo::ClientAppName, AppInfo::Version);
    md += QStringLiteral("| **Workload Build** | %1 (%2) |\n").arg(AppInfo::WorkloadTitle, AppInfo::WorkloadTag);
    md += QStringLiteral("| **OS Name** | %1 |\n").arg(QSysInfo::prettyProductName());
    md += QStringLiteral("| **OS Architecture** | %1 (%2) |\n").arg(QSysInfo::currentCpuArchitecture(), QSysInfo::buildCpuArchitecture());
    md += QStringLiteral("| **Kernel** | %1 %2 |\n").arg(QSysInfo::kernelType(), QSysInfo::kernelVersion());
    md += QStringLiteral("| **Logical CPU Cores** | %1 threads |\n").arg(QThread::idealThreadCount());

    // RAM stats
    quint64 totalRamMb = 0;
    quint64 availRamMb = 0;
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        totalRamMb = memInfo.ullTotalPhys / (1024 * 1024);
        availRamMb = memInfo.ullAvailPhys / (1024 * 1024);
    }
#elif defined(__linux__)
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        totalRamMb = (static_cast<quint64>(si.totalram) * si.mem_unit) / (1024 * 1024);
        availRamMb = (static_cast<quint64>(si.freeram) * si.mem_unit) / (1024 * 1024);
    }
#endif
    if (totalRamMb > 0) {
        md += QStringLiteral("| **System RAM** | %1 MB Total (%2 MB Available) |\n").arg(totalRamMb).arg(availRamMb);
    }

    // GPU stats
    QString gpuDesc = QStringLiteral("None / Basic");
    quint64 dedicatedVramMb = 0;
#ifdef _WIN32
    IDXGIFactory* factory = nullptr;
    if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory)))) {
        IDXGIAdapter* adapter = nullptr;
        for (UINT i = 0; SUCCEEDED(factory->EnumAdapters(i, &adapter)); ++i) {
            if (!adapter) continue;
            DXGI_ADAPTER_DESC desc;
            if (SUCCEEDED(adapter->GetDesc(&desc))) {
                if (desc.VendorId != 0x1414) { // Filter out Microsoft Basic Render Driver
                    quint64 vram = static_cast<quint64>(desc.DedicatedVideoMemory) / (1024 * 1024);
                    if (vram > dedicatedVramMb || gpuDesc == QStringLiteral("None / Basic")) {
                        dedicatedVramMb = vram;
                        gpuDesc = QString::fromWCharArray(desc.Description);
                    }
                }
            }
            adapter->Release();
        }
        factory->Release();
    }
#endif
    md += QStringLiteral("| **GPU Device** | %1 |\n").arg(gpuDesc);
    if (dedicatedVramMb > 0) {
        md += QStringLiteral("| **Dedicated VRAM** | %1 MB |\n").arg(dedicatedVramMb);
    }

    return md;
}

QString DiagnosticCollector::collectModelInfoMarkdown()
{
    QString md;
    md += QStringLiteral("| Component / Model | Required File | Status | File Size |\n");
    md += QStringLiteral("| --- | --- | --- | --- |\n");

    QString modelsDir = ModelDownloader::getModelsDirectory();

    // Gemma 2B
    QString gemmaPath = modelsDir + "/" + Models::LocalGemmaFile;
    QFileInfo gemmaInfo(gemmaPath);
    bool gemmaExists = gemmaInfo.exists() && gemmaInfo.size() > 0;
    md += QStringLiteral("| **Gemma 2 2B Instruct (LLM)** | `%1` | %2 | %3 |\n")
        .arg(Models::LocalGemmaFile)
        .arg(gemmaExists ? "✅ Present" : "❌ Missing")
        .arg(gemmaExists ? formatBytes(gemmaInfo.size()) : "—");

    // BGE Embedding
    QString bgePath = modelsDir + "/" + Models::LocalBgeEmbeddingFile;
    QFileInfo bgeInfo(bgePath);
    bool bgeExists = bgeInfo.exists() && bgeInfo.size() > 0;
    md += QStringLiteral("| **BGE Small v1.5 (Embedding)** | `%1` | %2 | %3 |\n")
        .arg(Models::LocalBgeEmbeddingFile)
        .arg(bgeExists ? "✅ Present" : "❌ Missing")
        .arg(bgeExists ? formatBytes(bgeInfo.size()) : "—");

    // Tesseract OCR model
    QString tessPath = QCoreApplication::applicationDirPath() + "/tessdata/eng.traineddata";
    if (!QFile::exists(tessPath)) {
        tessPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../tessdata/eng.traineddata");
    }
    QFileInfo tessInfo(tessPath);
    bool tessExists = tessInfo.exists() && tessInfo.size() > 0;
    md += QStringLiteral("| **Tesseract OCR English** | `eng.traineddata` | %1 | %2 |\n")
        .arg(tessExists ? "✅ Present" : "⚠️ Not Found in Default Location")
        .arg(tessExists ? formatBytes(tessInfo.size()) : "—");

    md += QStringLiteral("\n*Models storage directory: `%1`*\n").arg(modelsDir);
    return md;
}

QString DiagnosticCollector::collectLogsMarkdown(int maxLines)
{
    QString logDir = Logger::logDirectory();
    QDir dir(logDir);
    if (!dir.exists()) {
        return QStringLiteral("*No logs directory found at `%1`.*").arg(logDir);
    }

    QFileInfoList logFiles = dir.entryInfoList({ QStringLiteral("*.log") }, QDir::Files, QDir::Time);
    if (logFiles.isEmpty()) {
        return QStringLiteral("*No log files found in `%1`.*").arg(logDir);
    }

    QString combinedLogs;
    int remainingLines = maxLines;

    for (const QFileInfo &fileInfo : logFiles) {
        if (remainingLines <= 0) break;

        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

        QTextStream in(&file);
        QStringList lines;
        while (!in.atEnd()) {
            lines.append(in.readLine());
        }
        file.close();

        int takeCount = qMin(static_cast<qsizetype>(remainingLines), lines.size());
        QStringList tailLines = lines.mid(lines.size() - takeCount);

        if (!tailLines.isEmpty()) {
            combinedLogs += QStringLiteral("=== Log: %1 (Last %2 lines) ===\n")
                                .arg(fileInfo.fileName())
                                .arg(tailLines.size());
            combinedLogs += tailLines.join("\n") + "\n\n";
            remainingLines -= takeCount;
        }
    }

    if (combinedLogs.isEmpty()) {
        return QStringLiteral("*Log files are currently empty.*");
    }

    QString sanitized = sanitizeLogContent(combinedLogs);
    QString md;
    md += QStringLiteral("<details>\n<summary>📋 Recent Application Logs (click to expand)</summary>\n\n");
    md += QStringLiteral("```log\n");
    md += sanitized.trimmed();
    md += QStringLiteral("\n```\n</details>\n");
    return md;
}

QString DiagnosticCollector::buildBugReportMarkdown(
    const QString &description,
    const QString &stepsToReproduce,
    const QString &severity,
    bool includeLogs,
    bool includeSysInfo)
{
    QString md;
    md += QStringLiteral("### 🐞 Bug Description\n");
    md += description.trimmed().isEmpty() ? QStringLiteral("*No description provided.*") : description.trimmed();
    md += QStringLiteral("\n\n");

    if (!stepsToReproduce.trimmed().isEmpty()) {
        md += QStringLiteral("### 🔄 Steps to Reproduce\n");
        md += stepsToReproduce.trimmed();
        md += QStringLiteral("\n\n");
    }

    md += QStringLiteral("### ⚠️ Severity\n");
    md += severity.isEmpty() ? QStringLiteral("Normal") : severity;
    md += QStringLiteral("\n\n---\n\n");

    if (includeSysInfo) {
        md += QStringLiteral("<details open>\n<summary>💻 System Information</summary>\n\n");
        md += collectSystemInfoMarkdown();
        md += QStringLiteral("\n</details>\n\n");

        md += QStringLiteral("<details open>\n<summary>🤖 AI Model & Runtime Information</summary>\n\n");
        md += collectModelInfoMarkdown();
        md += QStringLiteral("\n</details>\n\n");
    }

    if (includeLogs) {
        md += collectLogsMarkdown(80);
        md += QStringLiteral("\n");
    }

    md += QStringLiteral("\n*Report generated automatically by Carbon ESG v%1 at %2*\n")
              .arg(AppInfo::Version, QDateTime::currentDateTime().toString(Qt::ISODate));

    return md;
}

QString DiagnosticCollector::buildFeedbackMarkdown(
    int rating,
    const QString &category,
    const QString &comments,
    bool includeSysInfo)
{
    QString md;
    md += QStringLiteral("### ⭐ Rating\n");
    QString stars;
    int clampedRating = qBound(1, rating, 5);
    for (int i = 0; i < clampedRating; ++i) stars += QStringLiteral("⭐ ");
    for (int i = clampedRating; i < 5; ++i) stars += QStringLiteral("☆ ");
    md += QStringLiteral("%1 (%2 / 5 Stars)\n\n").arg(stars).arg(clampedRating);

    md += QStringLiteral("### 🏷️ Category\n");
    md += category.trimmed().isEmpty() ? QStringLiteral("General Feedback") : category.trimmed();
    md += QStringLiteral("\n\n");

    md += QStringLiteral("### 💬 User Comments & Suggestions\n");
    md += comments.trimmed().isEmpty() ? QStringLiteral("*No additional comments provided.*") : comments.trimmed();
    md += QStringLiteral("\n\n---\n\n");

    if (includeSysInfo) {
        md += QStringLiteral("<details>\n<summary>💻 Environment Specifications</summary>\n\n");
        md += collectSystemInfoMarkdown();
        md += QStringLiteral("\n</details>\n\n");
    }

    md += QStringLiteral("*Submitted via Carbon ESG v%1 on %2*\n")
              .arg(AppInfo::Version, QDateTime::currentDateTime().toString(Qt::ISODate));

    return md;
}

QJsonObject DiagnosticCollector::collectDiagnosticsJson(bool includeLogs)
{
    QJsonObject root;
    root["app_name"] = AppInfo::ClientAppName;
    root["app_version"] = AppInfo::Version;
    root["workload_title"] = AppInfo::WorkloadTitle;
    root["workload_tag"] = AppInfo::WorkloadTag;
    root["os_name"] = QSysInfo::prettyProductName();
    root["os_arch"] = QSysInfo::currentCpuArchitecture();
    root["kernel"] = QStringLiteral("%1 %2").arg(QSysInfo::kernelType(), QSysInfo::kernelVersion());
    root["cpu_cores"] = QThread::idealThreadCount();

    QString modelsDir = ModelDownloader::getModelsDirectory();
    QJsonObject modelsObj;
    modelsObj["directory"] = modelsDir;
    modelsObj["gemma_present"] = QFile::exists(modelsDir + "/" + Models::LocalGemmaFile);
    modelsObj["bge_present"] = QFile::exists(modelsDir + "/" + Models::LocalBgeEmbeddingFile);
    root["models"] = modelsObj;

    if (includeLogs) {
        root["logs"] = collectLogsMarkdown(50);
    }

    return root;
}
