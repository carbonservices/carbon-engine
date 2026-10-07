#include "HTTPServer.h"

// Qt
#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonDocument>
#include <QtConcurrent>
#include <QTcpServer>
#include <QHostAddress>
#include <QUrlQuery>
#include <QFile>

// Templating
#include <inja/inja.hpp>
#include <nlohmann/json.hpp>

HttpServer::HttpServer(Database &db, LlamaEngine &llama, VectorIndex &index, QObject *parent)
    : QObject(parent), _db(db), _llama(llama), _index(index)
{
    // Clean route bindings targeting class methods
    _server.route("/api/ingest", QHttpServerRequest::Method::Post,
                  [this](const QHttpServerRequest &req)
                  { return handleIngest(req); });

    _server.route("/api/chat", QHttpServerRequest::Method::Post,
                  [this](const QHttpServerRequest &req)
                  { return handleChat(req); });

    _server.route("/api/report", QHttpServerRequest::Method::Post,
                  [this](const QHttpServerRequest &req)
                  { return handleReport(req); });

    _server.route("/api/jobs", QHttpServerRequest::Method::Get,
                  [this](const QHttpServerRequest &req)
                  { return handleJobStatus(req); });
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

QFuture<QHttpServerResponse> HttpServer::handleIngest(const QHttpServerRequest &request)
{
    auto doc = QJsonDocument::fromJson(request.body());
    QString filePath = doc.object().value("file_path").toString();

    // Offload to background thread
    return QtConcurrent::run([this, filePath]()
                             {
        if (filePath.isEmpty()) {
            _log.error("Ingest request missing file_path");
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
        }

        int jobId = _db.enqueueJob(filePath);
        if (jobId < 0) {
            _log.error(QString("Failed to enqueue job for: %1").arg(filePath));
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
        }
        
        _log.info(QString("Job %1 queued for: %2").arg(jobId).arg(filePath));
        return QHttpServerResponse(QJsonObject{{"status", "QUEUED"}, {"job_id", jobId}}); });
}

QFuture<QHttpServerResponse> HttpServer::handleChat(const QHttpServerRequest &request)
{
    auto doc = QJsonDocument::fromJson(request.body());
    QString prompt = doc.object().value("prompt").toString();

    return QtConcurrent::run([this, prompt]()
                             {
        if (prompt.isEmpty()) {
            _log.error("Chat request missing prompt");
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
        }

        _log.info("Chat request received");
        std::string promptStd = prompt.toStdString();
        
        auto queryEmbedding = _llama.generateEmbedding(promptStd);
        auto matchedIds = _index.search(queryEmbedding, 3);

        std::string generatedOutput;
        _llama.streamCompletion(promptStd, [&generatedOutput](const std::string& token) {
            generatedOutput += token;
        });

        _log.info("Chat response generated");
        return QHttpServerResponse(QJsonObject{
            {"response", QString::fromStdString(generatedOutput)},
            {"context_matches", static_cast<int>(matchedIds.size())}
        }); });
}

QFuture<QHttpServerResponse> HttpServer::handleReport(const QHttpServerRequest &request)
{
    auto doc = QJsonDocument::fromJson(request.body());
    int jobId = doc.object().value("job_id").toInt(-1);

    return QtConcurrent::run([this, jobId]()
                             {
        if (jobId < 0) {
            _log.error("Report request missing job_id");
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
        }

        _log.info(QString("RAG Search initiated for job %1").arg(jobId));

        // 1. Define the focus of the report
        std::string queryText = "What are the key Business Responsibility and Sustainability metrics, emissions, and environmental impacts?";
        
        // 2. Vector search
        std::vector<float> queryEmb = _llama.generateEmbedding(queryText);
        std::vector<int64_t> topVectorIds = _index.search(queryEmb, 4);
        
        // 3. Retrieve relevant chunks
        QString assembledContext = "";
        for (int64_t vid : topVectorIds) {
            assembledContext += _db.getChunkByVectorId(vid) + "\n\n";
        }

        if (assembledContext.isEmpty()) {
            _log.error(QString("No vector context found for job %1").arg(jobId));
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        }

        // 4. Format prompt exactly as Qwen Instruct expects, forcing a JSON output
        std::string finalPrompt = 
            "<|im_start|>system\n"
            "You are a strict data extraction AI. Extract the following metrics from the context into a flat JSON object. Output ONLY valid JSON. Use null if missing.\n"
            "Keys: entity_name, reporting_financial_year, scope_1_2_intensity_current, p6_water_intensity_turnover_current, emp_total_female_pct, csr_spent_amount, current_renewable_energy_pct.<|im_end|>\n"
            "<|im_start|>user\n"
            "Context:\n" + assembledContext.toStdString() + "\n\nExtract JSON.<|im_end|>\n"
            "<|im_start|>assistant\n{"; 

        _log.info(QString("Generating JSON extract for job %1...").arg(jobId));

        // 5. Generate the JSON string
        std::string generatedJson = "{"; 
        _llama.streamCompletion(finalPrompt, [&generatedJson](const std::string& token) {
            generatedJson += token;
        });

        // Strip any trailing ChatML tags that the model might append after the JSON
        size_t endPos = generatedJson.find("<|im_end|>");
        if (endPos != std::string::npos) {
            generatedJson = generatedJson.substr(0, endPos);
        }

        // 6. Render the HTML Template using Inja
        std::string renderedHtml;
        try {
            nlohmann::json data = nlohmann::json::parse(generatedJson);
            
            QFile file(QCoreApplication::applicationDirPath() + "/template.html");
            if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                std::string tmpl = file.readAll().toStdString();
                inja::Environment env;
                renderedHtml = env.render(tmpl, data);
            } else {
                renderedHtml = "<h1>Error: template.html not found on disk</h1>";
            }
        } catch(const std::exception& e) {
            _log.error(QString("JSON Parse Error: %1").arg(e.what()));
            renderedHtml = "<h1>Failed to parse LLM output</h1><pre>" + generatedJson + "</pre>";
        }

        _log.info(QString("Report generated for job %1").arg(jobId));
        return QHttpServerResponse(QJsonObject{
            {"report", QString::fromStdString(renderedHtml)}
        }); });
}

QFuture<QHttpServerResponse> HttpServer::handleJobStatus(const QHttpServerRequest &request)
{
    bool ok = false;
    int jobId = request.query().queryItemValue("job_id").toInt(&ok);

    if (!ok || jobId < 0)
    {
        _log.error("Job status request missing job_id query parameter");
        return QtFuture::makeReadyFuture(QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest));
    }

    QString status = _db.getJobStatus(jobId);
    if (status.isEmpty())
    {
        _log.error(QString("Job status requested for unknown job %1").arg(jobId));
        return QtFuture::makeReadyFuture(QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound));
    }

    return QtFuture::makeReadyFuture(QHttpServerResponse(QJsonObject{{"status", status}}));
}