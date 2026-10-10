#pragma once
#include <QStringList>
#include <QVector>
#include "db/Database.h"

// Flags issues in a job's extracted report sections before the user accepts them: missing
// required fields, out-of-range percentages, and simple cross-field consistency checks.
// Produces plain warning strings for the review UI, not hard errors — accepting a report
// with warnings is a user decision, not something this class enforces.
class ReportValidator {
public:
    static QStringList validate(const QVector<ReportSection>& sections);
};
