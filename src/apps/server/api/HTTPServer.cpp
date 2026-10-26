#include "HTTPServer.h"
#include "Hateoas.h"
#include "ServerConfig.h"
#include "constants/CommonConstants.h"
#include "report/ReportValidator.h"

// Qt
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QThreadPool>
#include <QTcpServer>
#include <QHostAddress>
#include <QUrlQuery>
#include <QFileInfo>
#include <QHttpHeaders>
#include <QFile>
#include <QCoreApplication>

static QHttpServerResponse jsonError(QHttpServerResponder::StatusCode code, const QString &message)
{
    QJsonObject obj;
    obj["error"] = message;
    obj["status_code"] = static_cast<int>(code);
    return QHttpServerResponse(obj, code);
}

void HttpServer::handleCorsAfterRequest(const QHttpServerRequest &, QHttpServerResponse &resp)
{
    QHttpHeaders headers = resp.headers();
    headers.replaceOrAppend("Access-Control-Allow-Origin", "*");
    headers.replaceOrAppend("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    headers.replaceOrAppend("Access-Control-Allow-Headers", "Content-Type, Authorization");
    resp.setHeaders(std::move(headers));
}

void HttpServer::handleMissingRequest(const QHttpServerRequest &req, QHttpServerResponder &responder)
{
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
}

QHttpServerResponse HttpServer::handleServeLogo(const QHttpServerRequest &)
{
    QString logoPath = QCoreApplication::applicationDirPath() + "/" + AppInfo::LogoFileName;
    QFile file(logoPath);
    if (!file.exists()) {
        file.setFileName(AppInfo::LogoFileName);
    }
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QHttpServerResponse resp("image/png", data);
        QHttpHeaders headers = resp.headers();
        headers.append("Access-Control-Allow-Origin", "*");
        headers.append("Cache-Control", "public, max-age=86400");
        resp.setHeaders(std::move(headers));
        return resp;
    }
    return jsonError(QHttpServerResponder::StatusCode::NotFound, "logo.png not found on disk");
}

HttpServer::HttpServer(Database &db, LlamaEngine &llama,
                       VectorIndexRegistry &indexRegistry,
                       SystemMonitor &monitor, const BRSRSchemaCatalog &catalog,
                       ReportExtractionService &extractionService, ReportExporter &exporter,
                       JobQueue &jobQueue,
                       QObject *parent)
    : QObject(parent), _db(db), _llama(llama), _indexRegistry(indexRegistry), _monitor(monitor),
      _catalog(catalog), _extractionService(extractionService), _exporter(exporter),
      _reportService(db, catalog), _jobQueue(jobQueue)
{
    // Global CORS middleware: inject Access-Control headers into all responses
    _server.addAfterRequestHandler(this, &HttpServer::handleCorsAfterRequest);

    // Global OPTIONS preflight and 404 JSON fallback handler
    _server.setMissingHandler(this, &HttpServer::handleMissingRequest);

    _server.route(ApiRoutes::Root, QHttpServerRequest::Method::Get, this, &HttpServer::handleApiRoot);
    _server.route("/", QHttpServerRequest::Method::Get, this, &HttpServer::handleApiRoot);
    _server.route(ApiRoutes::Ingest, QHttpServerRequest::Method::Post, this, &HttpServer::handleIngest);
    _server.route(ApiRoutes::Jobs, QHttpServerRequest::Method::Get, this, &HttpServer::handleJobStatus);
    _server.route(ApiRoutes::System, QHttpServerRequest::Method::Get, this, &HttpServer::handleSystemStatus);
    _server.route(ApiRoutes::Settings, QHttpServerRequest::Method::Get, this, &HttpServer::handleGetSettings);
    _server.route(ApiRoutes::Settings, QHttpServerRequest::Method::Post, this, &HttpServer::handlePostSettings);
    _server.route(ApiRoutes::ExtractionStart, QHttpServerRequest::Method::Post, this, &HttpServer::handleExtractionStart);
    _server.route(ApiRoutes::ExtractionStatus, QHttpServerRequest::Method::Get, this, &HttpServer::handleExtractionStatus);
    _server.route(ApiRoutes::ExtractionCancel, QHttpServerRequest::Method::Post, this, &HttpServer::handleExtractionCancel);
    _server.route(ApiRoutes::JobCancel, QHttpServerRequest::Method::Post, this, &HttpServer::handleJobCancel);
    _server.route(ApiRoutes::ExtractionSections, QHttpServerRequest::Method::Get, this, &HttpServer::handleExtractionSections);
    _server.route(ApiRoutes::ExtractionSections, QHttpServerRequest::Method::Put, this, &HttpServer::handleExtractionSectionUpdate);
    _server.route(ApiRoutes::ExtractionAccept, QHttpServerRequest::Method::Post, this, &HttpServer::handleExtractionAccept);
    _server.route(ApiRoutes::ReportExport, QHttpServerRequest::Method::Post, this, &HttpServer::handleReportExport);
    _server.route(ApiRoutes::Reports, QHttpServerRequest::Method::Get, this, &HttpServer::handleReportsList);
    _server.route(ApiRoutes::Reports, QHttpServerRequest::Method::Delete, this, &HttpServer::handleReportDelete);
    _server.route(ApiRoutes::Documents, QHttpServerRequest::Method::Get, this, &HttpServer::handleJobDocuments);
    _server.route(ApiRoutes::SchemaBrsr, QHttpServerRequest::Method::Get, this, &HttpServer::handleGetSchemaBrsr);

    _server.route("/logo.png", QHttpServerRequest::Method::Get, &HttpServer::handleServeLogo);
    _server.route("/api/logo", QHttpServerRequest::Method::Get, &HttpServer::handleServeLogo);
}

bool HttpServer::listen(quint16 port)
{
    auto *tcpServer = new QTcpServer(this);
    bool bindAll = (qEnvironmentVariableIntValue("CARBON_ENGINE_BIND_ALL") == 1);
    QHostAddress bindAddr = bindAll ? QHostAddress::Any : QHostAddress::LocalHost;
    if (!tcpServer->listen(bindAddr, port))
    {
        _log.error(QString("Failed to bind to port %1 on %2").arg(port).arg(bindAddr.toString()));
        delete tcpServer;
        return false;
    }

    bool bound = _server.bind(tcpServer);
    if (bound)
    {
        _log.info(QString("Listening on %1:%2").arg(bindAddr.toString()).arg(port));
    }
    return bound;
}

QFuture<QHttpServerResponse> HttpServer::handleApiRoot(const QHttpServerRequest &/*request*/)
{
    QJsonObject root;
    root["service"] = "Carbon Engine REST API";
    root["version"] = AppInfo::Version;
    root["workload"] = QString::fromUtf8(AppInfo::WorkloadTitle);
    root["_links"] = Hateoas::rootLinks();
    return QtFuture::makeReadyFuture(QHttpServerResponse(root));
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
    QJsonObject resp;
    resp["status"] = "QUEUED";
    resp["job_id"] = jobId;
    resp["_links"] = Hateoas::ingestLinks(jobId);
    return QtFuture::makeReadyFuture(QHttpServerResponse(resp));
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
    obj["documents"] = _reportService.getJobDocumentsJson(jobId);
    obj["_links"] = Hateoas::jobLinks(jobId, status, obj["extraction_status"].toString(), obj["report_locked"].toBool());

    return QtFuture::makeReadyFuture(QHttpServerResponse(obj));
}

QFuture<QHttpServerResponse> HttpServer::handleSystemStatus(const QHttpServerRequest& /*request*/)
{
    QJsonObject obj = _monitor.getSystemSnapshot();
    obj["gpu_offload_mode"]     = _llama.offloadModeString();
    obj["gpu_layers_offloaded"] = _llama.activeGenGpuLayers();
    obj["job_queue_depth"]      = _db.getPendingJobCount();

    quint64 dbBytes             = _db.diskSizeBytes();
    double dbMb                 = static_cast<double>(dbBytes) / (1024.0 * 1024.0);
    int totalChunks             = _db.totalChunkCount();
    obj["db_status"]            = totalChunks > 0
                                  ? QString("%1 MB (%2 chunks)").arg(QString::number(dbMb, 'f', 1)).arg(totalChunks)
                                  : QString("%1 MB").arg(QString::number(dbMb, 'f', 1));

    obj["cloud_enabled"]        = false;
    obj["active_engine_mode"]   = EngineMode::Offline;
    bool genModelLoaded = _llama.isGenModelLoaded();
    bool genModelPresent = QFile::exists(kGenerationModelPath);
    bool embdModelPresent = QFile::exists(kEmbeddingModelPath);

    QString activeModelName;
    if (genModelLoaded) {
        activeModelName = QFileInfo(kGenerationModelPath).fileName();
    } else if (_llama.isEmbdModelLoaded()) {
        activeModelName = QFileInfo(kEmbeddingModelPath).fileName();
    } else if (genModelPresent) {
        activeModelName = QString("Standby (%1)").arg(QFileInfo(kGenerationModelPath).fileName());
    } else {
        activeModelName = "None (Not Downloaded)";
    }

    obj["active_model"]         = activeModelName;
    obj["model_loaded"]         = genModelLoaded;
    obj["model_present"]        = genModelPresent;
    obj["embedding_model_present"] = embdModelPresent;
    obj["inference_mode"]          = _llama.activeInferenceModeString();
    obj["force_cpu"]               = _llama.isForceCpu();
    obj["force_gpu"]               = _llama.isForceGpu();
    obj["system_specs_precedence"]  = (!_llama.isForceCpu() && !_llama.isForceGpu());
    obj["workload_tag"]            = QString::fromUtf8(AppInfo::WorkloadTag);
    obj["workload_title"]          = QString::fromUtf8(AppInfo::WorkloadTitle);
    obj["_links"]                  = Hateoas::systemLinks();

    return QtFuture::makeReadyFuture(QHttpServerResponse(obj));
}

QFuture<QHttpServerResponse> HttpServer::handleGetSettings(const QHttpServerRequest& /*request*/)
{
    QJsonObject obj;
    bool forceCpu = _llama.isForceCpu();
    bool forceGpu = _llama.isForceGpu();
    bool systemSpecsPrecedence = (!forceCpu && !forceGpu);

    obj["force_cpu"] = forceCpu;
    obj["force_gpu"] = forceGpu;
    obj["system_specs_precedence"] = systemSpecsPrecedence;
    obj["active_mode"] = _llama.activeInferenceModeString();
    obj["gpu_present"] = _monitor.gpuPresent();
    obj["gpu_name"] = _monitor.gpuDeviceName();
    obj["vram_total_mb"] = static_cast<qint64>(_monitor.vramTotalMb());
    obj["workload_title"] = QString::fromUtf8(AppInfo::WorkloadTitle);
    obj["_links"] = Hateoas::settingsLinks();

    return QtFuture::makeReadyFuture(QHttpServerResponse(obj));
}

QFuture<QHttpServerResponse> HttpServer::handlePostSettings(const QHttpServerRequest& request)
{
    auto doc = QJsonDocument::fromJson(request.body());
    if (!doc.isObject()) {
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::BadRequest,
            "Invalid JSON payload for /api/settings"));
    }

    QJsonObject reqObj = doc.object();
    if (reqObj.contains("force_cpu")) {
        _llama.setForceCpu(reqObj.value("force_cpu").toBool());
    }
    if (reqObj.contains("force_gpu")) {
        _llama.setForceGpu(reqObj.value("force_gpu").toBool());
    }

    return handleGetSettings(request);
}

QFuture<QHttpServerResponse> HttpServer::handleExtractionStart(const QHttpServerRequest &request)
{
    auto doc = QJsonDocument::fromJson(request.body());
    int jobId = doc.object().value("job_id").toInt(-1);

    if (jobId < 0) {
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::BadRequest,
            "Missing or invalid 'job_id'"));
    }

    QString status = _db.getExtractionStatus(jobId);
    if (status == ExtractionStatus::Extracting) {
        QJsonObject resp;
        resp["job_id"] = jobId;
        resp["status"] = ExtractionStatus::Extracting;
        resp["_links"] = Hateoas::extractionStartLinks(jobId);
        return QtFuture::makeReadyFuture(QHttpServerResponse(resp));
    }

    if (!QFile::exists(kGenerationModelPath) || !QFile::exists(kEmbeddingModelPath)) {
        _log.error("Cannot start extraction: required AI model files are missing from disk.");
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::ServiceUnavailable,
            "AI model files not found on disk. Please run download-models or place GGUF models in '<Documents>/Carbon ESG/models/'."
        ));
    }

    _extractionService.startAsyncExtraction(jobId);

    QJsonObject resp;
    resp["job_id"] = jobId;
    resp["status"] = ExtractionStatus::Extracting;
    resp["_links"] = Hateoas::extractionStartLinks(jobId);
    return QtFuture::makeReadyFuture(QHttpServerResponse(resp));
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
    resp["engine_mode"] = EngineMode::Offline;
    resp["active_model"] = QFileInfo(kGenerationModelPath).fileName();

    auto progress = _extractionService.currentProgress();
    QJsonArray missingArr;
    if (_extractionService.activeJobId() == jobId || progress.currentStep > 0) {
        resp["current_step"] = progress.currentStep;
        resp["total_steps"] = progress.totalSteps;
        resp["current_group"] = progress.currentGroupId;
        resp["current_group_label"] = progress.currentGroupLabel;
        resp["progress_pct"] = progress.totalSteps > 0
            ? (static_cast<double>(progress.currentStep) / progress.totalSteps) * 100.0
            : 0.0;
        resp["last_latency_ms"] = progress.lastLatencyMs;

        for (const auto& sec : progress.missingSections) {
            QJsonObject secObj;
            secObj["group_id"] = sec.groupId;
            secObj["group_label"] = sec.groupLabel;
            QJsonArray keysArr;
            for (const auto& k : sec.missingKeys) {
                keysArr.append(k);
            }
            secObj["missing_keys"] = keysArr;
            missingArr.append(secObj);
        }
    } else {
        QVector<ReportSection> sections = _db.getReportSections(jobId);
        for (const auto& sec : sections) {
            const FieldGroup* grp = _catalog.findGroup(sec.groupId);
            if (!grp) continue;
            QJsonObject dataObj = QJsonDocument::fromJson(sec.dataJson.toUtf8()).object();
            QStringList missing;
            for (const QString& key : grp->allFieldKeys()) {
                if (!dataObj.contains(key) || dataObj.value(key).isNull() || dataObj.value(key).toString().trimmed().isEmpty()) {
                    missing.append(key);
                }
            }
            if (!missing.isEmpty()) {
                QJsonObject secObj;
                secObj["group_id"] = grp->id;
                secObj["group_label"] = grp->label;
                QJsonArray keysArr;
                for (const auto& k : missing) {
                    keysArr.append(k);
                }
                secObj["missing_keys"] = keysArr;
                missingArr.append(secObj);
            }
        }
    }
    resp["missing_metrics"] = missingArr;

    resp["_links"] = Hateoas::extractionStatusLinks(jobId, status);
    return QtFuture::makeReadyFuture(QHttpServerResponse(resp));
}

QFuture<QHttpServerResponse> HttpServer::handleExtractionCancel(const QHttpServerRequest &request)
{
    auto doc = QJsonDocument::fromJson(request.body());
    int jobId = doc.object().value("job_id").toInt(-1);
    if (jobId < 0) {
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::BadRequest, "Missing valid 'job_id' parameter"));
    }

    _log.warn(QString("Cancellation requested for extraction job %1").arg(jobId));
    _extractionService.cancelJob(jobId);
    _db.setExtractionStatus(jobId, ExtractionStatus::Cancelled);

    QJsonObject resp;
    resp["status"] = "CANCELLED";
    resp["job_id"] = jobId;
    resp["message"] = "Extraction halted. All in-flight tasks cancelled.";
    resp["_links"] = Hateoas::extractionCancelLinks(jobId);
    return QtFuture::makeReadyFuture(QHttpServerResponse(resp));
}

QFuture<QHttpServerResponse> HttpServer::handleJobCancel(const QHttpServerRequest &request)
{
    auto doc = QJsonDocument::fromJson(request.body());
    int jobId = doc.object().value("job_id").toInt(-1);
    if (jobId < 0) {
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::BadRequest, "Missing valid 'job_id' parameter"));
    }

    _log.warn(QString("Cancellation requested for document ingestion job %1").arg(jobId));
    _jobQueue.cancelJob(jobId);
    _db.updateJobStatus(jobId, JobStatus::Cancelled);

    QJsonObject resp;
    resp["status"] = "CANCELLED";
    resp["job_id"] = jobId;
    resp["message"] = "Ingestion halted. All in-flight tasks cancelled.";
    resp["_links"] = Hateoas::jobCancelLinks(jobId);
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

    QJsonObject resp = _reportService.getExtractionSections(jobId);
    bool isLocked = _db.isReportLocked(jobId);
    resp["is_locked"] = isLocked;
    resp["status"] = _db.getExtractionStatus(jobId);
    resp["_links"] = Hateoas::extractionSectionsLinks(jobId, isLocked);
    return QtFuture::makeReadyFuture(QHttpServerResponse(resp));
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

    QString errMsg;
    bool forbidden = false;
    if (!_reportService.updateSection(jobId, groupId, data, errMsg, forbidden)) {
        return QtFuture::makeReadyFuture(jsonError(
            forbidden ? QHttpServerResponder::StatusCode::Forbidden
                      : QHttpServerResponder::StatusCode::InternalServerError,
            errMsg));
    }

    QJsonObject resp;
    resp["status"] = ReportSectionStatus::Updated;
    resp["group_id"] = groupId;
    resp["job_id"] = jobId;
    resp["_links"] = Hateoas::extractionSectionUpdateLinks(jobId);
    return QtFuture::makeReadyFuture(QHttpServerResponse(resp));
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

    QString errMsg;
    if (!_reportService.acceptReport(jobId, errMsg)) {
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::InternalServerError,
            errMsg));
    }
    QJsonObject resp;
    resp["status"] = ExtractionStatus::Accepted;
    resp["job_id"] = jobId;
    resp["_links"] = Hateoas::extractionAcceptLinks(jobId);
    return QtFuture::makeReadyFuture(QHttpServerResponse(resp));
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
    resp["_links"] = Hateoas::reportExportLinks(jobId, isLocked);
    return QtFuture::makeReadyFuture(QHttpServerResponse(resp));
}

QFuture<QHttpServerResponse> HttpServer::handleReportsList(const QHttpServerRequest &request)
{
    bool ok = false;
    int limit = request.query().queryItemValue("limit").toInt(&ok);
    int offset = request.query().queryItemValue("offset").toInt();

    QJsonArray rawList = _reportService.listReports(ok ? limit : 0, offset);
    QJsonArray enrichedList = Hateoas::enrichReportsList(rawList);
    return QtFuture::makeReadyFuture(QHttpServerResponse(enrichedList));
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
    int waitCount = 0;
    while ((_jobQueue.currentJobId() == jobId || _extractionService.activeJobId() == jobId) && waitCount < 10) {
        QThread::msleep(100);
        waitCount++;
    }
    _extractionService.factStore().clearForJob(jobId);

    bool deleted = _db.deleteJob(jobId);
    if (!deleted) {
        return QtFuture::makeReadyFuture(jsonError(
            QHttpServerResponder::StatusCode::InternalServerError,
            QString("Failed to delete job %1 from database").arg(jobId)));
    }
    _indexRegistry.remove(jobId);
    _log.info(QString("Successfully deleted report / job %1").arg(jobId));
    QJsonObject resp;
    resp["status"] = "DELETED";
    resp["job_id"] = jobId;
    resp["_links"] = Hateoas::reportDeleteLinks(jobId);
    return QtFuture::makeReadyFuture(QHttpServerResponse(resp));
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

    QJsonObject resp;
    resp["job_id"] = jobId;
    resp["documents"] = _reportService.getJobDocumentsJson(jobId);
    resp["_links"] = Hateoas::jobDocumentsLinks(jobId);
    return QtFuture::makeReadyFuture(QHttpServerResponse(resp));
}

QFuture<QHttpServerResponse> HttpServer::handleGetSchemaBrsr(const QHttpServerRequest &request)
{
    Q_UNUSED(request);
    QJsonObject schemaObj = _catalog.schemaMetadata();
    schemaObj["_links"] = Hateoas::schemaBrsrLinks();
    return QtFuture::makeReadyFuture(QHttpServerResponse(schemaObj));
}

