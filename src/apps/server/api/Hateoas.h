#ifndef HATEOAS_H
#define HATEOAS_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>

// HATEOAS hypermedia link generator for Carbon Engine REST API.
// Provides lightweight, state-aware RFC 8288 / HAL-style hypermedia controls (_links).
class Hateoas
{
public:
    // Core link constructor: creates a {"href": "...", "method": "...", ["title": "..."]} object
    static QJsonObject makeLink(const QString& href, const QString& method, const QString& title = QString());

    // Helper to insert a link into a target links object
    static void addLink(QJsonObject& links, const QString& rel, const QString& href,
                        const QString& method, const QString& title = QString());

    // Endpoint-specific hypermedia link generators
    static QJsonObject rootLinks();
    static QJsonObject ingestLinks(int jobId);
    static QJsonObject jobLinks(int jobId, const QString& jobStatus,
                                const QString& extractionStatus, bool isReportLocked);
    static QJsonObject jobCancelLinks(int jobId);
    static QJsonObject systemLinks();
    static QJsonObject settingsLinks();
    static QJsonObject extractionStartLinks(int jobId);
    static QJsonObject extractionStatusLinks(int jobId, const QString& status);
    static QJsonObject extractionCancelLinks(int jobId);
    static QJsonObject extractionSectionsLinks(int jobId, bool isLocked);
    static QJsonObject extractionSectionUpdateLinks(int jobId);
    static QJsonObject extractionAcceptLinks(int jobId);
    static QJsonObject reportExportLinks(int jobId, bool isLocked);
    static QJsonObject reportItemLinks(int jobId, const QString& jobStatus,
                                       const QString& extractionStatus, bool isLocked);
    static QJsonObject reportDeleteLinks(int jobId);
    static QJsonObject jobDocumentsLinks(int jobId);
    static QJsonObject schemaBrsrLinks();

    // Enriches an array of report objects by attaching _links to each report item
    static QJsonArray enrichReportsList(const QJsonArray& reports);
};

#endif // HATEOAS_H

