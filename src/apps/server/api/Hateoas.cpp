#include "Hateoas.h"
#include "constants/CommonConstants.h"

QJsonObject Hateoas::makeLink(const QString& href, const QString& method, const QString& title)
{
    QJsonObject link;
    link["href"] = href;
    link["method"] = method;
    if (!title.isEmpty()) {
        link["title"] = title;
    }
    return link;
}

void Hateoas::addLink(QJsonObject& links, const QString& rel, const QString& href,
                      const QString& method, const QString& title)
{
    links[rel] = makeLink(href, method, title);
}

QJsonObject Hateoas::rootLinks()
{
    QJsonObject links;
    addLink(links, "self", ApiRoutes::Root, "GET", "Root API discovery");
    addLink(links, "system", ApiRoutes::System, "GET", "System telemetry and hardware status");
    addLink(links, "settings", ApiRoutes::Settings, "GET", "Hardware and inference testing settings");
    addLink(links, "ingest", ApiRoutes::Ingest, "POST", "Enqueue document files for processing");
    addLink(links, "jobs", ApiRoutes::Jobs, "GET", "Query status of an ingestion job");
    addLink(links, "reports", ApiRoutes::Reports, "GET", "List generated reports");
    addLink(links, "schema", ApiRoutes::SchemaBrsr, "GET", "BRSR schema and field metadata");
    return links;
}

QJsonObject Hateoas::ingestLinks(int jobId)
{
    QJsonObject links;
    QString jobUrl = QString("%1?job_id=%2").arg(ApiRoutes::Jobs).arg(jobId);
    QString docsUrl = QString("%1?job_id=%2").arg(ApiRoutes::Documents).arg(jobId);

    addLink(links, "self", jobUrl, "GET", "Check job status");
    addLink(links, "status", jobUrl, "GET", "Poll job progress");
    addLink(links, "cancel", ApiRoutes::JobCancel, "POST", "Cancel active ingestion");
    addLink(links, "documents", docsUrl, "GET", "View ingested documents");
    return links;
}

QJsonObject Hateoas::jobLinks(int jobId, const QString& jobStatus,
                              const QString& extractionStatus, bool isReportLocked)
{
    QJsonObject links;
    QString jobUrl = QString("%1?job_id=%2").arg(ApiRoutes::Jobs).arg(jobId);
    QString docsUrl = QString("%1?job_id=%2").arg(ApiRoutes::Documents).arg(jobId);
    QString extStatusUrl = QString("%1?job_id=%2").arg(ApiRoutes::ExtractionStatus).arg(jobId);
    QString sectionsUrl = QString("%1?job_id=%2").arg(ApiRoutes::ExtractionSections).arg(jobId);
    QString reportDelUrl = QString("%1?job_id=%2").arg(ApiRoutes::Reports).arg(jobId);

    addLink(links, "self", jobUrl, "GET", "Job status");
    addLink(links, "documents", docsUrl, "GET", "Associated documents");

    // State: Ingestion in progress
    if (jobStatus == JobStatus::Pending || jobStatus == JobStatus::Processing) {
        addLink(links, "cancel", ApiRoutes::JobCancel, "POST", "Cancel ingestion job");
    }

    // State: Ingestion completed
    if (jobStatus == JobStatus::Completed) {
        if (extractionStatus == ExtractionStatus::Extracting) {
            addLink(links, "extraction_status", extStatusUrl, "GET", "Poll extraction progress");
            addLink(links, "cancel_extraction", ApiRoutes::ExtractionCancel, "POST", "Cancel active extraction");
        } else if (extractionStatus.isEmpty() ||
                   extractionStatus == ExtractionStatus::NotStarted ||
                   extractionStatus == ExtractionStatus::Failed ||
                   extractionStatus == ExtractionStatus::Cancelled) {
            addLink(links, "start_extraction", ApiRoutes::ExtractionStart, "POST", "Start BRSR extraction");
        }

        if (extractionStatus == ExtractionStatus::ReadyForReview ||
            extractionStatus == ExtractionStatus::Completed ||
            extractionStatus == ExtractionStatus::Accepted) {
            addLink(links, "sections", sectionsUrl, "GET", "View extracted sections");
            addLink(links, "export_report", ApiRoutes::ReportExport, "POST", "Export report HTML");

            if (!isReportLocked && extractionStatus != ExtractionStatus::Accepted) {
                addLink(links, "update_section", ApiRoutes::ExtractionSections, "PUT", "Edit section data");
                addLink(links, "accept_report", ApiRoutes::ExtractionAccept, "POST", "Accept and lock report");
            }
        }

        addLink(links, "delete_report", reportDelUrl, "DELETE", "Delete report and index");
    }

    return links;
}

QJsonObject Hateoas::jobCancelLinks(int jobId)
{
    QJsonObject links;
    addLink(links, "self", QString("%1?job_id=%2").arg(ApiRoutes::Jobs).arg(jobId), "GET", "Job details");
    addLink(links, "reports", ApiRoutes::Reports, "GET", "Reports list");
    addLink(links, "ingest", ApiRoutes::Ingest, "POST", "Queue new ingestion");
    return links;
}

QJsonObject Hateoas::systemLinks()
{
    QJsonObject links;
    addLink(links, "self", ApiRoutes::System, "GET", "Refresh system status");
    addLink(links, "settings", ApiRoutes::Settings, "GET", "View inference settings");
    addLink(links, "update_settings", ApiRoutes::Settings, "POST", "Update settings overrides");
    addLink(links, "reports", ApiRoutes::Reports, "GET", "List reports");
    addLink(links, "ingest", ApiRoutes::Ingest, "POST", "Ingest documents");
    return links;
}

QJsonObject Hateoas::settingsLinks()
{
    QJsonObject links;
    addLink(links, "self", ApiRoutes::Settings, "GET", "Fetch current settings");
    addLink(links, "update", ApiRoutes::Settings, "POST", "Update settings overrides");
    addLink(links, "system", ApiRoutes::System, "GET", "System telemetry");
    return links;
}

QJsonObject Hateoas::extractionStartLinks(int jobId)
{
    QJsonObject links;
    addLink(links, "self", QString("%1?job_id=%2").arg(ApiRoutes::ExtractionStatus).arg(jobId), "GET", "Extraction status");
    addLink(links, "status", QString("%1?job_id=%2").arg(ApiRoutes::ExtractionStatus).arg(jobId), "GET", "Poll extraction");
    addLink(links, "cancel", ApiRoutes::ExtractionCancel, "POST", "Cancel extraction");
    addLink(links, "job", QString("%1?job_id=%2").arg(ApiRoutes::Jobs).arg(jobId), "GET", "Job status");
    return links;
}

QJsonObject Hateoas::extractionStatusLinks(int jobId, const QString& status)
{
    QJsonObject links;
    addLink(links, "self", QString("%1?job_id=%2").arg(ApiRoutes::ExtractionStatus).arg(jobId), "GET", "Current status");
    addLink(links, "job", QString("%1?job_id=%2").arg(ApiRoutes::Jobs).arg(jobId), "GET", "Job status");

    if (status == ExtractionStatus::Extracting) {
        addLink(links, "cancel", ApiRoutes::ExtractionCancel, "POST", "Cancel extraction");
    } else if (status == ExtractionStatus::ReadyForReview ||
               status == ExtractionStatus::Completed ||
               status == ExtractionStatus::Accepted) {
        addLink(links, "sections", QString("%1?job_id=%2").arg(ApiRoutes::ExtractionSections).arg(jobId), "GET", "View sections");
        addLink(links, "export", ApiRoutes::ReportExport, "POST", "Export report");
        if (status != ExtractionStatus::Accepted) {
            addLink(links, "accept", ApiRoutes::ExtractionAccept, "POST", "Accept report");
        }
    } else if (status == ExtractionStatus::Cancelled || status == ExtractionStatus::Failed) {
        addLink(links, "retry", ApiRoutes::ExtractionStart, "POST", "Retry extraction");
    }
    return links;
}

QJsonObject Hateoas::extractionCancelLinks(int jobId)
{
    QJsonObject links;
    addLink(links, "self", QString("%1?job_id=%2").arg(ApiRoutes::Jobs).arg(jobId), "GET", "Job status");
    addLink(links, "retry", ApiRoutes::ExtractionStart, "POST", "Retry extraction");
    addLink(links, "reports", ApiRoutes::Reports, "GET", "Reports list");
    return links;
}

QJsonObject Hateoas::extractionSectionsLinks(int jobId, bool isLocked)
{
    QJsonObject links;
    addLink(links, "self", QString("%1?job_id=%2").arg(ApiRoutes::ExtractionSections).arg(jobId), "GET", "Current sections");
    addLink(links, "schema", ApiRoutes::SchemaBrsr, "GET", "BRSR schema metadata");
    addLink(links, "job", QString("%1?job_id=%2").arg(ApiRoutes::Jobs).arg(jobId), "GET", "Parent job");
    addLink(links, "export", ApiRoutes::ReportExport, "POST", "Export report HTML");

    if (!isLocked) {
        addLink(links, "update_section", ApiRoutes::ExtractionSections, "PUT", "Update field data");
        addLink(links, "accept", ApiRoutes::ExtractionAccept, "POST", "Accept and lock report");
    }
    return links;
}

QJsonObject Hateoas::extractionSectionUpdateLinks(int jobId)
{
    QJsonObject links;
    addLink(links, "self", QString("%1?job_id=%2").arg(ApiRoutes::ExtractionSections).arg(jobId), "GET", "View sections");
    addLink(links, "update_section", ApiRoutes::ExtractionSections, "PUT", "Update another section");
    addLink(links, "accept", ApiRoutes::ExtractionAccept, "POST", "Accept report");
    addLink(links, "export", ApiRoutes::ReportExport, "POST", "Export report");
    addLink(links, "job", QString("%1?job_id=%2").arg(ApiRoutes::Jobs).arg(jobId), "GET", "Parent job");
    return links;
}

QJsonObject Hateoas::extractionAcceptLinks(int jobId)
{
    QJsonObject links;
    addLink(links, "self", QString("%1?job_id=%2").arg(ApiRoutes::ExtractionSections).arg(jobId), "GET", "Locked sections");
    addLink(links, "export", ApiRoutes::ReportExport, "POST", "Export final report");
    addLink(links, "job", QString("%1?job_id=%2").arg(ApiRoutes::Jobs).arg(jobId), "GET", "Job status");
    addLink(links, "reports", ApiRoutes::Reports, "GET", "All reports");
    return links;
}

QJsonObject Hateoas::reportExportLinks(int jobId, bool isLocked)
{
    QJsonObject links;
    addLink(links, "self", ApiRoutes::ReportExport, "POST", "Re-export report");
    addLink(links, "sections", QString("%1?job_id=%2").arg(ApiRoutes::ExtractionSections).arg(jobId), "GET", "Review sections");
    addLink(links, "job", QString("%1?job_id=%2").arg(ApiRoutes::Jobs).arg(jobId), "GET", "Job status");
    if (!isLocked) {
        addLink(links, "accept", ApiRoutes::ExtractionAccept, "POST", "Accept and lock report");
    }
    return links;
}

QJsonObject Hateoas::reportItemLinks(int jobId, const QString& /*jobStatus*/,
                                     const QString& extractionStatus, bool /*isLocked*/)
{
    QJsonObject links;
    addLink(links, "self", QString("%1?job_id=%2").arg(ApiRoutes::Jobs).arg(jobId), "GET", "Job status");
    addLink(links, "documents", QString("%1?job_id=%2").arg(ApiRoutes::Documents).arg(jobId), "GET", "Documents");

    if (extractionStatus == ExtractionStatus::ReadyForReview ||
        extractionStatus == ExtractionStatus::Completed ||
        extractionStatus == ExtractionStatus::Accepted) {
        addLink(links, "sections", QString("%1?job_id=%2").arg(ApiRoutes::ExtractionSections).arg(jobId), "GET", "Sections");
        addLink(links, "export", ApiRoutes::ReportExport, "POST", "Export report");
    }
    addLink(links, "delete", QString("%1?job_id=%2").arg(ApiRoutes::Reports).arg(jobId), "DELETE", "Delete report");
    return links;
}

QJsonObject Hateoas::reportDeleteLinks(int /*jobId*/)
{
    QJsonObject links;
    addLink(links, "reports", ApiRoutes::Reports, "GET", "Reports list");
    addLink(links, "ingest", ApiRoutes::Ingest, "POST", "New ingestion");
    addLink(links, "system", ApiRoutes::System, "GET", "System telemetry");
    return links;
}

QJsonObject Hateoas::jobDocumentsLinks(int jobId)
{
    QJsonObject links;
    addLink(links, "self", QString("%1?job_id=%2").arg(ApiRoutes::Documents).arg(jobId), "GET", "Document list");
    addLink(links, "job", QString("%1?job_id=%2").arg(ApiRoutes::Jobs).arg(jobId), "GET", "Parent job");
    return links;
}

QJsonObject Hateoas::schemaBrsrLinks()
{
    QJsonObject links;
    addLink(links, "self", ApiRoutes::SchemaBrsr, "GET", "Schema metadata");
    addLink(links, "reports", ApiRoutes::Reports, "GET", "Reports list");
    addLink(links, "system", ApiRoutes::System, "GET", "System telemetry");
    return links;
}

QJsonArray Hateoas::enrichReportsList(const QJsonArray& reports)
{
    QJsonArray result;
    for (const QJsonValue& val : reports) {
        if (!val.isObject()) {
            result.append(val);
            continue;
        }
        QJsonObject obj = val.toObject();
        int jobId = obj.value("job_id").toInt(CoreConstants::InvalidJobId);
        QString jobStatus = obj.value("status").toString();
        QString extStatus = obj.value("extraction_status").toString();
        bool isLocked = obj.value("report_locked").toBool();
        obj["_links"] = reportItemLinks(jobId, jobStatus, extStatus, isLocked);
        result.append(obj);
    }
    return result;
}

