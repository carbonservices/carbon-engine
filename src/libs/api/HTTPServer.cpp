#include "HTTPServer.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QtConcurrent>
#include <QTcpServer>
#include <QHostAddress>
#include <QUrlQuery>
#include <QFuture>

namespace
{

    QFuture<QHttpServerResponse> handleIngest(Database &db, const QHttpServerRequest &request)
    {
        auto doc = QJsonDocument::fromJson(request.body());
        QString filePath = doc.object().value("file_path").toString();
        Logger log("IngestRoute");

        return QtConcurrent::run([&db, filePath, log]()
                                 {
        if (filePath.isEmpty()) {
            log.error("Ingest request missing file_path");
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
        }

        int jobId = db.enqueueJob(filePath);
        if (jobId < 0) {
            log.error(QString("Failed to enqueue job for: %1").arg(filePath));
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
        }
        log.info(QString("Job %1 queued for: %2").arg(jobId).arg(filePath));
        return QHttpServerResponse(QJsonObject{{"status", "QUEUED"}, {"job_id", jobId}}); });
    }

    QFuture<QHttpServerResponse> handleChat(LlamaEngine &llama, VectorIndex &index, const QHttpServerRequest &request)
    {
        auto doc = QJsonDocument::fromJson(request.body());
        QString prompt = doc.object().value("prompt").toString();
        Logger log("ChatRoute");

        return QtConcurrent::run([&llama, &index, prompt, log]()
                                 {
        if (prompt.isEmpty()) {
            log.error("Chat request missing prompt");
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
        }

        log.info("Chat request received");
        std::string promptStd = prompt.toStdString();
        auto queryEmbedding = llama.generateEmbedding(promptStd);
        auto matchedIds = index.search(queryEmbedding, 3);

        std::string generatedOutput;
        llama.streamCompletion(promptStd, [&generatedOutput](const std::string& token) {
            generatedOutput += token;
        });

        log.info("Chat response generated");
        return QHttpServerResponse(QJsonObject{
            {"response", QString::fromStdString(generatedOutput)},
            {"context_matches", static_cast<int>(matchedIds.size())}
        }); });
    }

    QFuture<QHttpServerResponse> handleReport(Database &db, LlamaEngine &llama, const QHttpServerRequest &request)
    {
        auto doc = QJsonDocument::fromJson(request.body());
        int jobId = doc.object().value("job_id").toInt(-1);
        Logger log("ReportRoute");

        return QtConcurrent::run([&db, &llama, jobId, log]()
                                 {
        if (jobId < 0) {
            log.error("Report request missing job_id");
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
        }

        auto chunks = db.getChunksForJob(jobId);
        if (chunks.isEmpty()) {
            log.error(QString("No chunks found for job %1").arg(jobId));
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        }

        log.info(QString("Generating report for job %1 from %2 chunks").arg(jobId).arg(chunks.size()));
        QString context = chunks.join("\n\n");
        QString prompt = QString(
            "Using the following extracted document content, produce a BRSR-style "
            "ESG report summarizing the key disclosures.\n\n%1").arg(context);

        std::string generatedOutput;
        llama.streamCompletion(prompt.toStdString(), [&generatedOutput](const std::string& token) {
            generatedOutput += token;
        });

        log.info(QString("Report generated for job %1").arg(jobId));
        return QHttpServerResponse(QJsonObject{
            {"report", QString::fromStdString(generatedOutput)}
        }); });
    }

    QFuture<QHttpServerResponse> handleJobStatus(Database &db, const QHttpServerRequest &request)
    {
        Logger log("JobStatusRoute");

        bool ok = false;
        int jobId = request.query().queryItemValue("job_id").toInt(&ok);
        if (!ok || jobId < 0)
        {
            log.error("Job status request missing job_id query parameter");
            return QtFuture::makeReadyFuture(QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest));
        }

        QString status = db.getJobStatus(jobId);
        if (status.isEmpty())
        {
            log.error(QString("Job status requested for unknown job %1").arg(jobId));
            return QtFuture::makeReadyFuture(QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound));
        }
        return QtFuture::makeReadyFuture(QHttpServerResponse(QJsonObject{{"status", status}}));
    }

} // namespace

HttpServer::HttpServer(Database &db, LlamaEngine &llama, VectorIndex &index, QObject *parent)
    : QObject(parent), _db(db), _llama(llama), _index(index)
{
    _server.route("/api/ingest", QHttpServerRequest::Method::Post,
                  [this](const QHttpServerRequest &request)
                  { return handleIngest(_db, request); });

    _server.route("/api/chat", QHttpServerRequest::Method::Post,
                  [this](const QHttpServerRequest &request)
                  { return handleChat(_llama, _index, request); });

    _server.route("/api/report", QHttpServerRequest::Method::Post,
                  [this](const QHttpServerRequest &request)
                  { return handleReport(_db, _llama, request); });

    _server.route("/api/jobs", QHttpServerRequest::Method::Get,
                  [this](const QHttpServerRequest &request)
                  { return handleJobStatus(_db, request); });
}

bool HttpServer::listen(quint16 port)
{
    auto *tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, port))
    {
        _log.error(QString("Failed to bind to port %1").arg(port));
        return false;
    }
    bool bound = _server.bind(tcpServer);
    if (bound)
    {
        _log.info(QString("Listening on port %1").arg(port));
    }
    return bound;
}
