#include "ReportValidator.h"
#include <QString>
#include <nlohmann/json.hpp>
#include <cmath>

namespace {

bool tryParseNumber(const nlohmann::json& value, double& outNumber)
{
    if (value.is_number()) {
        outNumber = value.get<double>();
        return true;
    }
    if (value.is_string()) {
        try {
            std::string s = value.get<std::string>();
            std::string clean;
            for (char c : s) {
                if ((c >= '0' && c <= '9') || c == '.' || c == '-') {
                    clean += c;
                }
            }
            if (!clean.empty()) {
                outNumber = std::stod(clean);
                return true;
            }
        } catch (...) {
            return false;
        }
    }
    return false;
}

void checkMissingAndRangeIssues(const QString& groupId, const nlohmann::json& data, QStringList& warnings)
{
    for (auto& [key, value] : data.items()) {
        QString fieldName = QString::fromStdString(key);

        bool isMissing = value.is_null() ||
            (value.is_string() && (value.get<std::string>().empty() || value.get<std::string>() == "N/A"));
        if (isMissing) {
            warnings.append(QString("%1: field \"%2\" has no extracted value").arg(groupId, fieldName));
            continue;
        }

        if (fieldName.endsWith("_pct")) {
            double number = 0.0;
            if (tryParseNumber(value, number) && (number < 0.0 || number > 100.0)) {
                warnings.append(QString("%1: field \"%2\" is %3, outside expected 0-100%% range")
                    .arg(groupId, fieldName).arg(number));
            }
        }
    }
}

void checkWorkforceConsistency(const QString& groupId, const nlohmann::json& data, QStringList& warnings)
{
    if (data.contains("emp_total_male_pct") && data.contains("emp_total_female_pct")) {
        double malePct = 0.0;
        double femalePct = 0.0;
        if (tryParseNumber(data["emp_total_male_pct"], malePct) && tryParseNumber(data["emp_total_female_pct"], femalePct)) {
            double total = malePct + femalePct;
            if (total < 95.0 || total > 105.0) {
                warnings.append(QString("%1: employee gender breakdown sum (%2% + %3% = %4%) deviates from 100%")
                    .arg(groupId).arg(malePct).arg(femalePct).arg(total));
            }
        }
    }

    if (data.contains("emp_perm_total") && data.contains("emp_other_total") && data.contains("emp_total_total")) {
        double perm = 0.0;
        double other = 0.0;
        double total = 0.0;
        if (tryParseNumber(data["emp_perm_total"], perm) && tryParseNumber(data["emp_other_total"], other) && tryParseNumber(data["emp_total_total"], total)) {
            if (total > 0.0 && std::abs((perm + other) - total) > 1.0) {
                warnings.append(QString("%1: employee headcount mismatch: permanent (%2) + other (%3) != total (%4)")
                    .arg(groupId).arg(perm).arg(other).arg(total));
            }
        }
    }
}

} // namespace

QStringList ReportValidator::validate(const QVector<ReportSection>& sections)
{
    QStringList warnings;
    for (const ReportSection& section : sections) {
        if (section.status == "FAILED") {
            warnings.append(QString("%1: extraction failed, no values available").arg(section.groupId));
            continue;
        }

        try {
            nlohmann::json data = nlohmann::json::parse(section.dataJson.toStdString());
            checkMissingAndRangeIssues(section.groupId, data, warnings);
            if (section.groupId == "section_a_workforce" || section.groupId == "employees_workers") {
                checkWorkforceConsistency(section.groupId, data, warnings);
            }
        } catch (...) {
            warnings.append(QString("%1: stored data could not be parsed").arg(section.groupId));
        }
    }
    return warnings;
}
