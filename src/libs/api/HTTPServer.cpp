#include "HTTPServer.h"
#include "report/ReportValidator.h"

// Qt
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtConcurrent>
#include <QThreadPool>
#include <QTcpServer>
#include <QHostAddress>
#include <QUrlQuery>
#include <QFileInfo>
#include <QHttpHeaders>

// Templating
#include <nlohmann/json.hpp>

namespace {
QHttpServerResponse jsonError(QHttpServerResponder::StatusCode code, const QString &message)
{
    QJsonObject obj;
    obj["error"] = message;
    obj["status_code"] = static_cast<int>(code);
    return QHttpServerResponse(obj, code);
}
} // namespace

HttpServer::HttpServer(Database &db, LlamaEngine &llama, VectorIndexRegistry &indexRegistry,
                       SystemMonitor &monitor, const BrsrSchemaCatalog &catalog,
                       ReportExtractionService &extractionService, ReportExporter &exporter,
                       JobQueue &jobQueue,
                       QObject *parent)
    : QObject(parent), _db(db), _llama(llama), _indexRegistry(indexRegistry), _monitor(monitor),
      _catalog(catalog), _extractionService(extractionService), _exporter(exporter),
      _jobQueue(jobQueue)
{
    // Global CORS middleware: inject Access-Control headers into all responses
    _server.addAfterRequestHandler(this, [](const QHttpServerRequest &, QHttpServerResponse &resp) {
        QHttpHeaders headers = resp.headers();
        headers.replaceOrAppend("Access-Control-Allow-Origin", "*");
        headers.replaceOrAppend("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        headers.replaceOrAppend("Access-Control-Allow-Headers", "Content-Type, Authorization");
        resp.setHeaders(std::move(headers));
    });

    // Global OPTIONS preflight and 404 JSON fallback handler
    _server.setMissingHandler(this, [](const QHttpServerRequest &req, QHttpServerResponder &responder) {
        if (req.method() == QHttpServerRequest::Method::Options) {
            QHttpServerResponse resp(QHttpServerResponder::StatusCode::NoContent);
            QHttpHeaders headers;
            headers.append("Access-Control-Allow-Origin", "*");
            headers.append("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            headers.append("Access-Control-Allow-Headers", "Content-Type, Authorization");
            headers.append("Access-Control-Max-Age", "86400");
            resp.setHeaders(std::move(headers));
            responder.sendResponse(std::move(resp));
            return;
        }
        QJsonObject err;
        err["error"] = "Not Found";
        err["status_code"] = 404;
        QHttpServerResponse resp(err, QHttpServerResponder::StatusCode::NotFound);
        QHttpHeaders headers = resp.headers();
        headers.append("Access-Control-Allow-Origin", "*");
        resp.setHeaders(std::move(headers));
        responder.sendResponse(std::move(resp));
    });

    _server.route("/api/ingest", QHttpServerRequest::Method::Post,
                  [this](const QHttpServerRequest &req)
                  { return handleIngest(req); });

    _server.route("/api/jobs", QHttpServerRequest::Method::Get,
                  [this](const QHttpServerRequest &req)
                  { return handleJobStatus(req); });

    _server.route("/api/system", QHttpServerRequest::Method::Get,
                  [this](const QHttpServerRequest &req)
                  { return handleSystemStatus(req); });

    _server.route("/api/extraction/start", QHttpServerRequest::Method::Post,
                  [this](const QHttpServerRequest &req)
                  { return handleExtractionStart(req); });

    _server.route("/api/extraction/status", QHttpServerRequest::Method::Get,
                  [this](const QHttpServerRequest &req)
                  { return handleExtractionStatus(req); });

    _server.route("/api/extraction/sections", QHttpServerRequest::Method::Get,
                  [this](const QHttpServerRequest &req)
                  { return handleExtractionSections(req); });

    _server.route("/api/extraction/sections", QHttpServerRequest::Method::Put,
                  [this](const QHttpServerRequest &req)
                  { return handleExtractionSectionUpdate(req); });

    _server.route("/api/extraction/accept", QHttpServerRequest::Method::Post,
                  [this](const QHttpServerRequest &req)
                  { return handleExtractionAccept(req); });

    _server.route("/api/report/export", QHttpServerRequest::Method::Post,
                  [this](const QHttpServerRequest &req)
                  { return handleReportExport(req); });

    _server.route("/api/reports", QHttpServerRequest::Method::Get,
                  [this](const QHttpServerRequest &req)
                  { return handleReportsList(req); });

    _server.route("/api/reports", QHttpServerRequest::Method::Delete,
                  [this](const QHttpServerRequest &req)
                  { return handleReportDelete(req); });

    _server.route("/api/documents", QHttpServerRequest::Method::Get,
                  [this](const QHttpServerRequest &req)
                  { return handleJobDocuments(req); });

    auto serveLogo = [](const QHttpServerRequest &) {
        QFile file("logo.png");
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            QHttpServerResponse resp("image/png", data);
            QHttpHeaders headers = resp.headers();
            headers.append("Access-Control-Allow-Origin", "*");
            headers.append("Cache-Control", "public, max-age=86400");
            resp.setHeaders(std::move(headers));
            return resp;
        }
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
    };
    _server.route("/logo.png", QHttpServerRequest::Method::Get, serveLogo);
    _server.route("/api/logo", QHttpServerRequest::Method::Get, serveLogo);
}

bool HttpServer::listen(quint16 port)
{
    auto *tcpServer = new QTcpServer(this);
    bool bindAll = (qEnvironmentVariableIntValue("CARBON_ENGINE_BIND_ALL") == 1);
    QHostAddress bindAddr = bindAll ? QHostAddress::Any : QHostAddress::LocalHost;
    if (!tcpServer->listen(bindAddr, port))
    {
        _log.error(QString("Failed to bind to port %1 on %2").arg(port).arg(bindAddr.toString()));
        return false;
    }

    bool bound = _server.bind(tcpServer);
    if (bound)
    {
        _log.info(QString("Listening on %1:%2").arg(bindAddr.toString()).arg(port));
    }
    return bound;
}

QFuture<QHttpServerResponse> HttpServer::handleIngest(const QHttpServerRequest &request)
{
    auto doc = QJsonDocument::fromJson(request.body());
    QJsonArray pathArray = doc.object().value("file_paths").toArray();

    QStringList filePaths;
    for (const QJsonValue& value : pathArray) {
        if (value.isString() && !value.toString().trimmed().isEmpty()) {
            filePaths.append(value.toString().trimmed());
        }
    }

    if (filePaths.isEmpty()) {
        _log.error("Ingest request missing file_paths");
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::BadRequest,
            "Ingest request missing valid 'file_paths' array"));
    }

    // Verify all specified files exist on the filesystem before creating the job
    for (const QString& path : filePaths) {
        QFileInfo fi(path);
        if (!fi.exists() || !fi.isFile()) {
            _log.error(QString("Ingest file not found on disk: %1").arg(path));
            return QtFuture::makeReadyFuture(jsonError(
                QHttpServerResponder::StatusCode::BadRequest,
                QString("File not found on disk: %1").arg(path)));
        }
    }

    int jobId = _db.enqueueJob(filePaths);
    if (jobId < 0) {
        _log.error(QString("Failed to enqueue job for %1 document(s)").arg(filePaths.size()));
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::InternalServerError,
            "Failed to persist ingestion job to database"));
    }

    _log.info(QString("Job %1 queued for %2 document(s)").arg(jobId).arg(filePaths.size()));
    _jobQueue.notifyNewJob();
    return QtFuture::makeReadyFuture(QHttpServerResponse(QJsonObject{{"status", "QUEUED"}, {"job_id", jobId}}));
}

QFuture<QHttpServerResponse> HttpServer::handleJobStatus(const QHttpServerRequest &request)
{
    bool ok = false;
    int jobId = request.query().queryItemValue("job_id").toInt(&ok);

    if (!ok || jobId < 0)
    {
        _log.error("Job status request missing job_id query parameter");
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::BadRequest,
            "Missing or invalid 'job_id' query parameter"));
    }

    QString status = _db.getJobStatus(jobId);
    if (status.isEmpty())
    {
        _log.error(QString("Job status requested for unknown job %1").arg(jobId));
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::NotFound,
            QString("Job %1 not found").arg(jobId)));
    }

    QJsonObject obj;
    obj["job_id"] = jobId;
    obj["status"] = status;
    obj["extraction_status"] = _db.getExtractionStatus(jobId);
    obj["report_locked"] = _db.isReportLocked(jobId);

    auto jobDocs = _db.getJobDocuments(jobId);
    QJsonArray docsArr;
    for (const auto& d : jobDocs) {
        QJsonObject dObj;
        dObj["id"] = d.id;
        dObj["file_path"] = d.filePath;
        dObj["file_name"] = d.fileName;
        dObj["chunk_count"] = d.chunkCount;
        dObj["page_count"] = d.pageCount;
        docsArr.append(dObj);
    }
    obj["documents"] = docsArr;

    return QtFuture::makeReadyFuture(QHttpServerResponse(obj));
}

QFuture<QHttpServerResponse> HttpServer::handleSystemStatus(const QHttpServerRequest& /*request*/)
{
    double  cpuProc    = _monitor.cpuPctProcess();
    double  cpuSys     = _monitor.cpuPctSystem();
    quint64 memProc    = _monitor.memUsedMbProcess();
    quint64 memSys     = _monitor.memUsedMbSystem();
    quint64 memTotal   = _monitor.memTotalMbSystem();
    bool    gpuPres    = _monitor.gpuPresent();
    double  tps        = _monitor.tokensPerSec();
    QString phase      = SystemMonitor::phaseString(_monitor.enginePhase());
    int     queueDepth = _db.getPendingJobCount();

    QJsonObject obj;
    obj["cpu_pct_process"]        = qRound(cpuProc * 10.0) / 10.0;
    obj["cpu_pct_system"]         = qRound(cpuSys  * 10.0) / 10.0;
    obj["memory_used_mb_process"] = static_cast<qint64>(memProc);
    obj["memory_used_mb_system"]  = static_cast<qint64>(memSys);
    obj["memory_total_mb_system"] = static_cast<qint64>(memTotal);
    obj["gpu_present"]            = gpuPres;
    obj["vram_total_mb"]          = static_cast<qint64>(_monitor.vramTotalMb());
    obj["gpu_offload_mode"]       = _llama.offloadModeString();
    obj["gpu_layers_offloaded"]   = _llama.activeGenGpuLayers();
    obj["gpu_pct"]                = gpuPres ? QJsonValue(0.0) : QJsonValue(QJsonValue::Null);
    obj["job_queue_depth"]        = queueDepth;
    obj["engine_status"]          = phase;

    quint64 dbBytes               = _db.diskSizeBytes();
    double dbMb                   = static_cast<double>(dbBytes) / (1024.0 * 1024.0);
    int totalChunks               = _db.totalChunkCount();
    obj["db_status"]              = totalChunks > 0
                                    ? QString("%1 MB (%2 chunks)").arg(QString::number(dbMb, 'f', 1)).arg(totalChunks)
                                    : QString("%1 MB").arg(QString::number(dbMb, 'f', 1));

    obj["tokens_per_sec"]         = qRound(tps * 10.0) / 10.0;

    return QtFuture::makeReadyFuture(QHttpServerResponse(obj));
}

QFuture<QHttpServerResponse> HttpServer::handleExtractionStart(const QHttpServerRequest &request)
{
    auto doc = QJsonDocument::fromJson(request.body());
    int jobId = doc.object().value("job_id").toInt(-1);

    if (jobId < 0) {
        _log.error("Extraction start request missing valid job_id");
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::BadRequest,
            "Extraction start request missing valid 'job_id'"));
    }

    QString jobStatus = _db.getJobStatus(jobId);
    if (jobStatus.isEmpty()) {
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::NotFound,
            QString("Job %1 not found").arg(jobId)));
    }

    if (jobStatus != "COMPLETED") {
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::BadRequest,
            QString("Cannot extract job %1 with status '%2' (ingestion must be COMPLETED)").arg(jobId).arg(jobStatus)));
    }

    if (_extractionService.isExtracting()) {
        _log.warn(QString("Extraction already in progress for job %1. Rejecting request for job %2.")
                      .arg(_extractionService.activeJobId()).arg(jobId));
        QJsonObject errObj;
        errObj["error"] = "Extraction already in progress";
        errObj["active_job_id"] = _extractionService.activeJobId();
        errObj["status_code"] = static_cast<int>(QHttpServerResponder::StatusCode::Conflict);
        return QtFuture::makeReadyFuture(QHttpServerResponse(errObj, QHttpServerResponder::StatusCode::Conflict));
    }

    _log.info(QString("Starting extraction for job %1").arg(jobId));
    QThreadPool::globalInstance()->start([this, jobId]() { _extractionService.extractJob(jobId); });
    return QtFuture::makeReadyFuture(QHttpServerResponse(QJsonObject{{"status", "EXTRACTING"}, {"job_id", jobId}}));
}

QFuture<QHttpServerResponse> HttpServer::handleExtractionStatus(const QHttpServerRequest &request)
{
    bool ok = false;
    int jobId = request.query().queryItemValue("job_id").toInt(&ok);

    if (!ok || jobId < 0) {
        _log.error("Extraction status request missing job_id query parameter");
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::BadRequest,
            "Missing or invalid 'job_id' query parameter"));
    }

    QString status = _db.getExtractionStatus(jobId);
    if (status.isEmpty()) {
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::NotFound,
            QString("Job %1 not found").arg(jobId)));
    }

    QJsonObject resp;
    resp["job_id"] = jobId;
    resp["status"] = status;

    auto progress = _extractionService.currentProgress();
    if (_extractionService.activeJobId() == jobId || progress.currentStep > 0) {
        resp["current_step"] = progress.currentStep;
        resp["total_steps"] = progress.totalSteps;
        resp["current_group"] = progress.currentGroupLabel;
        resp["current_group_id"] = progress.currentGroupId;
        double pct = (progress.totalSteps > 0)
            ? (static_cast<double>(progress.currentStep) / progress.totalSteps * 100.0)
            : 0.0;
        resp["progress_pct"] = qRound(pct * 10.0) / 10.0;

        QJsonArray missingArr;
        for (const auto& sec : progress.missingSections) {
            QJsonObject sObj;
            sObj["group_id"] = sec.groupId;
            sObj["group_label"] = sec.groupLabel;
            sObj["missing_keys"] = QJsonArray::fromStringList(sec.missingKeys);
            missingArr.append(sObj);
        }
        resp["missing_metrics"] = missingArr;
    }

    return QtFuture::makeReadyFuture(QHttpServerResponse(resp));
}

QFuture<QHttpServerResponse> HttpServer::handleExtractionSections(const QHttpServerRequest &request)
{
    bool ok = false;
    int jobId = request.query().queryItemValue("job_id").toInt(&ok);

    if (!ok || jobId < 0) {
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::BadRequest,
            "Missing or invalid 'job_id' query parameter"));
    }

    if (_db.getJobStatus(jobId).isEmpty()) {
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::NotFound,
            QString("Job %1 not found").arg(jobId)));
    }

    QVector<ReportSection> sections = _db.getReportSections(jobId);

    QJsonArray sectionsJson;
    for (const ReportSection& section : sections) {
        const FieldGroup* group = _catalog.findGroup(section.groupId);
        QJsonObject obj;
        obj["group_id"] = section.groupId;
        obj["label"] = group ? group->label : section.groupId;
        obj["section"] = group ? group->section : QString();
        obj["status"] = section.status;
        obj["data"] = QJsonDocument::fromJson(section.dataJson.toUtf8()).object();
        if (group) {
            obj["data_keys"] = QJsonArray::fromStringList(group->dataFieldKeys);
            obj["narrative_keys"] = QJsonArray::fromStringList(group->narrativeFieldKeys);
            obj["loop_keys"] = QJsonArray::fromStringList(group->loopFieldKeys);
        }
        sectionsJson.append(obj);
    }

    QStringList warnings = ReportValidator::validate(sections);

    return QtFuture::makeReadyFuture(QHttpServerResponse(QJsonObject{
        {"sections", sectionsJson},
        {"warnings", QJsonArray::fromStringList(warnings)},
        {"job_id", jobId}
    }));
}

QFuture<QHttpServerResponse> HttpServer::handleExtractionSectionUpdate(const QHttpServerRequest &request)
{
    auto doc = QJsonDocument::fromJson(request.body());
    int jobId = doc.object().value("job_id").toInt(-1);
    QString groupId = doc.object().value("group_id").toString();
    QJsonObject data = doc.object().value("data").toObject();

    if (jobId < 0 || groupId.isEmpty()) {
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::BadRequest,
            "Missing valid 'job_id' or 'group_id' in request body"));
    }
    if (_db.isReportLocked(jobId)) {
        _log.error(QString("Rejected section edit for locked job %1").arg(jobId));
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::Forbidden,
            QString("Report for job %1 is locked and cannot be edited").arg(jobId)));
    }

    QString dataJson = QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Compact));
    if (!_db.upsertReportSection(jobId, groupId, dataJson, "DRAFT")) {
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::InternalServerError,
            "Failed to update report section in database"));
    }

    return QtFuture::makeReadyFuture(QHttpServerResponse(QJsonObject{{"status", "UPDATED"}, {"group_id", groupId}}));
}

QFuture<QHttpServerResponse> HttpServer::handleExtractionAccept(const QHttpServerRequest &request)
{
    auto doc = QJsonDocument::fromJson(request.body());
    int jobId = doc.object().value("job_id").toInt(-1);

    if (jobId < 0) {
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::BadRequest,
            "Extraction accept request missing valid 'job_id'"));
    }
    if (!_db.lockReport(jobId)) {
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::InternalServerError,
            "Failed to lock report in database"));
    }
    _log.info(QString("Report accepted and locked for job %1").arg(jobId));
    return QtFuture::makeReadyFuture(QHttpServerResponse(QJsonObject{{"status", "ACCEPTED"}, {"job_id", jobId}}));
}

QFuture<QHttpServerResponse> HttpServer::handleReportExport(const QHttpServerRequest &request)
{
    auto doc = QJsonDocument::fromJson(request.body());
    int jobId = doc.object().value("job_id").toInt(-1);
    bool allowDraft = doc.object().value("draft").toBool(true);

    if (jobId < 0) {
        _log.error("Report export request missing job_id");
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::BadRequest,
            "Report export request missing valid 'job_id'"));
    }

    bool isLocked = _db.isReportLocked(jobId);
    QString html = _exporter.exportJob(jobId, allowDraft || isLocked);
    QJsonObject resp;
    resp["report"] = html;
    resp["job_id"] = jobId;
    resp["is_locked"] = isLocked;
    return QtFuture::makeReadyFuture(QHttpServerResponse(resp));
}

QFuture<QHttpServerResponse> HttpServer::handleReportsList(const QHttpServerRequest &request)
{
    bool ok = false;
    int limit = request.query().queryItemValue("limit").toInt(&ok);
    int offset = request.query().queryItemValue("offset").toInt();

    auto reports = _db.getAllReports();
    QJsonArray arr;
    int count = 0;
    int skipped = 0;

    for (const auto& r : reports) {
        if (offset > 0 && skipped < offset) {
            skipped++;
            continue;
        }
        if (ok && limit > 0 && count >= limit) {
            break;
        }

        QJsonObject obj;
        obj["job_id"] = r.jobId;
        obj["title"] = r.title;
        obj["status"] = r.status;
        obj["extraction_status"] = r.extractionStatus;
        obj["report_locked"] = _db.isReportLocked(r.jobId);
        obj["date"] = r.createdAt.section(' ', 0, 0);

        auto jobDocs = _db.getJobDocuments(r.jobId);
        QJsonArray docs;
        for (const auto& d : jobDocs) {
            QJsonObject dObj;
            dObj["id"] = d.id;
            dObj["file_path"] = d.filePath;
            dObj["file_name"] = d.fileName;
            dObj["chunk_count"] = d.chunkCount;
            dObj["page_count"] = d.pageCount;
            docs.append(dObj);
        }
        obj["documents"] = docs;
        arr.append(obj);
        count++;
    }
    return QtFuture::makeReadyFuture(QHttpServerResponse(arr));
}

QFuture<QHttpServerResponse> HttpServer::handleReportDelete(const QHttpServerRequest &request)
{
    bool ok = false;
    int jobId = request.query().queryItemValue("job_id").toInt(&ok);
    if (!ok || jobId < 0) {
        _log.error("Report delete request missing valid job_id");
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::BadRequest,
            "Missing or invalid 'job_id' query parameter"));
    }

    // Cancel active ingestion or extraction before deletion
    _jobQueue.cancelJob(jobId);
    _extractionService.cancelJob(jobId);
    _extractionService.factStore().clearForJob(jobId);

    bool deleted = _db.deleteJob(jobId);
    if (!deleted) {
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::InternalServerError,
            QString("Failed to delete job %1 from database").arg(jobId)));
    }
    _indexRegistry.remove(jobId);
    _log.info(QString("Successfully deleted report / job %1").arg(jobId));
    return QtFuture::makeReadyFuture(QHttpServerResponse(QJsonObject{{"status", "DELETED"}, {"job_id", jobId}}));
}

QFuture<QHttpServerResponse> HttpServer::handleJobDocuments(const QHttpServerRequest &request)
{
    bool ok = false;
    int jobId = request.query().queryItemValue("job_id").toInt(&ok);

    if (!ok || jobId < 0) {
        _log.error("Job documents request missing job_id query parameter");
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::BadRequest,
            "Missing or invalid 'job_id' query parameter"));
    }

    if (_db.getJobStatus(jobId).isEmpty()) {
        _log.error(QString("Job documents requested for unknown job %1").arg(jobId));
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::NotFound,
            QString("Job %1 not found").arg(jobId)));
    }

    auto docs = _db.getJobDocuments(jobId);
    QJsonArray docsArr;
    for (const auto& d : docs) {
        QJsonObject dObj;
        dObj["id"] = d.id;
        dObj["job_id"] = d.jobId;
        dObj["file_path"] = d.filePath;
        dObj["file_name"] = d.fileName;
        dObj["chunk_count"] = d.chunkCount;
        dObj["page_count"] = d.pageCount;
        docsArr.append(dObj);
    }

    return QtFuture::makeReadyFuture(QHttpServerResponse(QJsonObject{
        {"job_id", jobId},
        {"documents", docsArr}
    }));
}

