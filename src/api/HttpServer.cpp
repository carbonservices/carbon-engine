#include "HttpServer.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QtConcurrent>

HttpServer::HttpServer(std::shared_ptr<Database> db,
                       std::shared_ptr<LlamaEngine> llama,
                       std::shared_ptr<VectorIndex> index,
                       QObject* parent)
    : QObject(parent), m_db(db), m_llama(llama), m_index(index) {

    // Enqueue document parsing job
    m_server.route("/api/ingest", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest& request) {
            auto doc = QJsonDocument::fromJson(request.body());
            QString filePath = doc.object().value("file_path").toString();

            if (filePath.isEmpty() || !m_db->enqueueJob(filePath)) {
                return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
            }
            return QHttpServerResponse(QJsonObject{{"status", "QUEUED"}});
        });

    // Chat / Retrieval RAG query
    m_server.route("/api/chat", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest& request) {
            auto doc = QJsonDocument::fromJson(request.body());
            std::string prompt = doc.object().value("prompt").toString().toStdString();

            auto queryEmbedding = m_llama->generateEmbedding(prompt);
            auto matchedIds = m_index->search(queryEmbedding, 3);

            // Synchronous completion payload (streamable via SSE / chunked responses if needed)
            std::string generatedOutput;
            m_llama->streamCompletion(prompt, [&generatedOutput](const std::string& token) {
                generatedOutput += token;
            });

            return QHttpServerResponse(QJsonObject{
                {"response", QString::fromStdString(generatedOutput)},
                {"context_matches", static_cast<int>(matchedIds.size())}
            });
        });
}

bool HttpServer::listen(quint16 port) {
    return m_server.listen(QHostAddress::Any, port) != 0;
}