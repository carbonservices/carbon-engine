#include "ReportExporter.h"

// Qt
#include <QCoreApplication>
#include <QFile>

// Templating
#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

namespace {

// Safe recursive merge that does not delete default fields when the extracted value is null,
// and ensures array structures are preserved cleanly without breaking template placeholders.
void safeMerge(nlohmann::json& target, const nlohmann::json& patch)
{
    if (!patch.is_object()) return;
    for (auto it = patch.begin(); it != patch.end(); ++it) {
        const std::string& key = it.key();
        const auto& val = it.value();
        if (val.is_null()) {
            // Keep target's safe default placeholder
            continue;
        }
        if (val.is_string()) {
            std::string s = val.get<std::string>();
            if (s.empty() || s == "null" || s == "N/A") {
                target[key] = "N/A";
            } else {
                target[key] = s;
            }
        } else if (val.is_array()) {
            if (!val.empty() || !target.contains(key) || !target[key].is_array()) {
                target[key] = val;
            }
        } else if (val.is_object()) {
            if (target.contains(key) && target[key].is_object()) {
                safeMerge(target[key], val);
            } else {
                target[key] = val;
            }
        } else {
            target[key] = val;
        }
    }
}

} // namespace

ReportExporter::ReportExporter(Database& db, const BrsrSchemaCatalog& catalog) : _db(db), _catalog(catalog) {}

QString ReportExporter::exportJob(int jobId, bool allowDraft)
{
    if (!allowDraft && !_db.isReportLocked(jobId)) {
        _log.error(QString("Export requested for job %1 before it was accepted").arg(jobId));
        return "<h1>Error: report has not been accepted yet</h1>";
    }

    QString tmplPath = QCoreApplication::applicationDirPath() + "/template.html";
    if (!QFile::exists(tmplPath)) {
        tmplPath = "template.html";
    }
    QFile file(tmplPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        _log.error("template.html not found on disk");
        return "<h1>Error: template.html not found on disk</h1>";
    }

    nlohmann::json reportData = _catalog.defaultSchema();
    for (const ReportSection& section : _db.getReportSections(jobId)) {
        if (section.status == "FAILED") {
            continue;
        }
        try {
            nlohmann::json sectionData = nlohmann::json::parse(section.dataJson.toStdString());
            safeMerge(reportData, sectionData);
        } catch (const std::exception& e) {
            _log.error(QString("Job %1 section %2 could not be merged: %3").arg(jobId).arg(section.groupId).arg(e.what()));
        }
    }

    // Ensure all loop/table collections are valid JSON arrays (prevents JS syntax error in chart scripts)
    for (const auto& group : _catalog.groups()) {
        for (const auto& loopKey : group.loopFieldKeys) {
            std::string keyStd = loopKey.toStdString();
            if (!reportData.contains(keyStd) || !reportData[keyStd].is_array()) {
                reportData[keyStd] = nlohmann::json::array();
            }
        }
    }

    try {
        std::string tmpl = file.readAll().toStdString();
        inja::Environment env;

        // Variadic range supporting both range(count) and range(start, end)
        env.add_callback("range", [](inja::Arguments& args) {
            int start = 0;
            int end = 0;
            if (args.size() == 1) {
                end = args.at(0)->get<int>();
            } else if (args.size() >= 2) {
                start = args.at(0)->get<int>();
                end = args.at(1)->get<int>();
            }
            nlohmann::json res = nlohmann::json::array();
            for (int i = start; i < end; ++i) {
                res.push_back(i);
            }
            return res;
        });

        // Float parser supporting numbers and formatted numeric strings (with commas/symbols)
        env.add_callback("float", 1, [](inja::Arguments& args) {
            try {
                if (args.at(0)->is_number()) {
                    return args.at(0)->get<double>();
                }
                if (args.at(0)->is_string()) {
                    std::string s = args.at(0)->get<std::string>();
                    std::string clean;
                    for (char c : s) {
                        if ((c >= '0' && c <= '9') || c == '.' || c == '-') clean += c;
                    }
                    if (!clean.empty()) return std::stod(clean);
                }
            } catch (...) {}
            return 0.0;
        });

        std::string renderedHtml = env.render(tmpl, reportData);
        _log.info(QString("Report exported for job %1 (allowDraft: %2)").arg(jobId).arg(allowDraft));
        return QString::fromStdString(renderedHtml);
    } catch (const std::exception& e) {
        _log.error(QString("Template render error for job %1: %2").arg(jobId).arg(e.what()));
        return QString("<h1>Template Rendering Error</h1><p>%1</p>").arg(e.what());
    }
}
