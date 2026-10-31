#include "Database.h"

// Qt
#include <QSqlQuery>
#include <QSqlError>
#include <QStringList>
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QThread>

Database::Database(const QString& dbPath) : _dbPath(dbPath) {}

Database::~Database() {}

QSqlDatabase Database::getDb() {
    // Use the memory address of the current thread ID as a unique connection name
    QString connName = QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    
    static QMutex s_connMutex;
    QMutexLocker locker(&s_connMutex);

    if (QSqlDatabase::contains(connName)) {
        return QSqlDatabase::database(connName);
    }

    auto db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(_dbPath);

    if (!db.open()) {
        _log.error(QString("SQLite connection failed on thread %1: %2").arg(connName, db.lastError().text()));
    } else {
        // OPTIMIZATION: Drastically improves concurrent read/write speeds
        QSqlQuery q(db);
        q.exec("PRAGMA journal_mode = WAL;");
        q.exec("PRAGMA synchronous = NORMAL;");
        q.exec("PRAGMA temp_store = MEMORY;");
        q.exec("PRAGMA busy_timeout = 5000;"); // Wait up to 5s instead of failing instantly if locked
        q.exec("PRAGMA foreign_keys = ON;"); // Must be set per-connection; SQLite does not enforce FKs otherwise
    }
    return db;
}

bool Database::init() {
    if (!runMigrations()) {
        return false;
    }
    return reconcileStuckJobs();
}

bool Database::reconcileStuckJobs() {
    QSqlQuery q(getDb());
    if (q.exec("UPDATE ingestion_jobs SET status = 'FAILED', error_message = 'Interrupted by server shutdown' WHERE status = 'PROCESSING';")) {
        int count = q.numRowsAffected();
        if (count > 0) {
            _log.warn(QString("Reconciled %1 interrupted ingestion job(s) from previous session").arg(count));
        }
    }
    if (q.exec("UPDATE job_documents SET status = 'FAILED' WHERE status = 'PROCESSING';")) {
        int count = q.numRowsAffected();
        if (count > 0) {
            _log.warn(QString("Reconciled %1 interrupted document(s) from previous session").arg(count));
        }
    }
    if (q.exec("UPDATE ingestion_jobs SET extraction_status = 'FAILED' WHERE extraction_status = 'EXTRACTING';")) {
        int count = q.numRowsAffected();
        if (count > 0) {
            _log.warn(QString("Reconciled %1 interrupted extraction job(s) from previous session").arg(count));
        }
    }
    return true;
}

bool Database::transaction() {
    return getDb().transaction();
}

bool Database::commit() {
    return getDb().commit();
}

bool Database::rollback() {
    return getDb().rollback();
}

bool Database::runMigrations() {
    QSqlQuery query(getDb());
    query.exec("PRAGMA foreign_keys = ON;");

    const QString schema = R"(
        CREATE TABLE IF NOT EXISTS ingestion_jobs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            status TEXT CHECK(status IN ('PENDING', 'PROCESSING', 'COMPLETED', 'FAILED', 'CANCELLED')) DEFAULT 'PENDING',
            error_message TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            extraction_status TEXT DEFAULT 'NOT_STARTED',
            report_locked_at DATETIME
        );
        CREATE INDEX IF NOT EXISTS idx_ingestion_jobs_status ON ingestion_jobs(status);

        CREATE TABLE IF NOT EXISTS job_documents (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            job_id INTEGER NOT NULL,
            file_path TEXT NOT NULL,
            document_index INTEGER NOT NULL,
            status TEXT DEFAULT 'QUEUED',
            FOREIGN KEY(job_id) REFERENCES ingestion_jobs(id) ON DELETE CASCADE
        );
        CREATE INDEX IF NOT EXISTS idx_job_documents_job_id ON job_documents(job_id);

        CREATE TABLE IF NOT EXISTS document_chunks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            job_id INTEGER NOT NULL,
            document_id INTEGER NOT NULL,
            page_num INTEGER,
            chunk_text TEXT,
            embedding BLOB,
            embedding_model TEXT,
            FOREIGN KEY(job_id) REFERENCES ingestion_jobs(id) ON DELETE CASCADE,
            FOREIGN KEY(document_id) REFERENCES job_documents(id) ON DELETE CASCADE
        );
        CREATE INDEX IF NOT EXISTS idx_document_chunks_job_id ON document_chunks(job_id);
        CREATE INDEX IF NOT EXISTS idx_document_chunks_document_id ON document_chunks(document_id);

        CREATE TABLE IF NOT EXISTS report_sections (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            job_id INTEGER NOT NULL,
            group_id TEXT NOT NULL,
            data_json TEXT NOT NULL,
            status TEXT CHECK(status IN ('DRAFT', 'FAILED', 'UPDATED')) DEFAULT 'DRAFT',
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            source_chunk_ids TEXT,
            FOREIGN KEY(job_id) REFERENCES ingestion_jobs(id) ON DELETE CASCADE,
            UNIQUE(job_id, group_id)
        );
        CREATE INDEX IF NOT EXISTS idx_report_sections_job_id ON report_sections(job_id);

        CREATE TABLE IF NOT EXISTS job_facts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            job_id INTEGER NOT NULL,
            metric_key TEXT NOT NULL,
            metric_value TEXT NOT NULL,
            source_chunk_id INTEGER,
            page_number INTEGER DEFAULT 0,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(job_id) REFERENCES ingestion_jobs(id) ON DELETE CASCADE,
            FOREIGN KEY(source_chunk_id) REFERENCES document_chunks(id) ON DELETE CASCADE,
            UNIQUE(job_id, metric_key)
        );
        CREATE INDEX IF NOT EXISTS idx_job_facts_job_key ON job_facts(job_id, metric_key);
    )";

    transaction();
    const QStringList statements = schema.split(';', Qt::SkipEmptyParts);
    for (const QString& statement : statements) {
        const QString trimmed = statement.trimmed();
        if (trimmed.isEmpty())
            continue;
        if (!query.exec(trimmed)) {
            _log.error(QString("Failed to initialize database schema: %1").arg(query.lastError().text()));
            rollback();
            return false;
        }
    }
    commit();
    return true;
}

int Database::enqueueJob(const QStringList& filePaths) {
    QSqlDatabase db = getDb();
    transaction();

    QSqlQuery jobQuery(db);
    jobQuery.prepare("INSERT INTO ingestion_jobs (status) VALUES ('PENDING');");
    if (!jobQuery.exec()) {
        rollback();
        return CoreConstants::InvalidJobId;
    }
    int jobId = jobQuery.lastInsertId().toInt();

    QSqlQuery docQuery(db);
    docQuery.prepare("INSERT INTO job_documents (job_id, file_path, document_index, status) VALUES (:job, :path, :idx, 'QUEUED');");
    for (int i = 0; i < filePaths.size(); ++i) {
        docQuery.bindValue(":job", jobId);
        docQuery.bindValue(":path", filePaths[i]);
        docQuery.bindValue(":idx", i);
        if (!docQuery.exec()) {
            rollback();
            return CoreConstants::InvalidJobId;
        }
    }

    if (!commit()) return CoreConstants::InvalidJobId;
    return jobId;
}

bool Database::getNextPendingJob(int& outJobId, QVector<JobDocument>& outDocuments) {
    QSqlQuery jobQuery(getDb());
    if (!jobQuery.exec("SELECT id FROM ingestion_jobs WHERE status = 'PENDING' ORDER BY id ASC LIMIT 1;") || !jobQuery.next()) {
        return false;
    }
    outJobId = jobQuery.value(0).toInt();

    outDocuments.clear();
    QSqlQuery docQuery(getDb());
    docQuery.prepare("SELECT id, file_path, COALESCE(status, 'QUEUED') FROM job_documents WHERE job_id = :job ORDER BY document_index ASC;");
    docQuery.bindValue(":job", outJobId);
    if (docQuery.exec()) {
        while (docQuery.next()) {
            outDocuments.append({docQuery.value(0).toInt(), docQuery.value(1).toString(), docQuery.value(2).toString()});
        }
    }
    return true;
}

bool Database::updateJobStatus(int jobId, const QString& status, const QString& error) {
    QSqlQuery q(getDb());
    q.prepare("UPDATE ingestion_jobs SET status = :st, error_message = :err WHERE id = :id;");
    q.bindValue(":st", status);
    q.bindValue(":err", error);
    q.bindValue(":id", jobId);
    return q.exec();
}

bool Database::updateDocumentStatus(int documentId, const QString& status) {
    QSqlQuery q(getDb());
    q.prepare("UPDATE job_documents SET status = :st WHERE id = :id;");
    q.bindValue(":st", status);
    q.bindValue(":id", documentId);
    return q.exec();
}

QVector<int64_t> Database::insertChunksBatch(int jobId, const std::vector<ChunkRecord>& chunks, const QString& embeddingModel) {
    QVector<int64_t> insertedIds;
    if (chunks.empty()) return insertedIds;

    QSqlQuery q(getDb());
    q.prepare("INSERT INTO document_chunks (job_id, document_id, page_num, chunk_text, embedding, embedding_model) "
              "VALUES (:job, :doc, :page, :text, :emb, :model);");

    for (const auto& ch : chunks) {
        q.bindValue(":job", jobId);
        q.bindValue(":doc", ch.documentId);
        q.bindValue(":page", ch.pageNumber);
        q.bindValue(":text", ch.text);

        QByteArray blob;
        if (!ch.embedding.empty()) {
            blob = QByteArray(reinterpret_cast<const char*>(ch.embedding.data()),
                              static_cast<qsizetype>(ch.embedding.size() * sizeof(float)));
        }
        q.bindValue(":emb", blob);
        q.bindValue(":model", embeddingModel);

        if (!q.exec()) {
            _log.error(QString("Batch chunk insert failed for job %1: %2").arg(jobId).arg(q.lastError().text()));
            return insertedIds;
        }
        insertedIds.append(q.lastInsertId().toLongLong());
    }
    _log.info(QString("SQLite: Inserted batch of %1 chunk(s) and binary embedding BLOBs for Job %2 (model: %3)")
        .arg(insertedIds.size()).arg(jobId).arg(embeddingModel));
    return insertedIds;
}

bool Database::upsertFact(int jobId, const QString& key, const QString& value, int64_t chunkId, int pageNumber) {
    QSqlQuery q(getDb());
    q.prepare("INSERT INTO job_facts (job_id, metric_key, metric_value, source_chunk_id, page_number) "
              "VALUES (:job, :key, :val, :chunk, :page) "
              "ON CONFLICT(job_id, metric_key) DO UPDATE SET "
              "metric_value = excluded.metric_value, source_chunk_id = excluded.source_chunk_id, "
              "page_number = excluded.page_number;");
    q.bindValue(":job", jobId);
    q.bindValue(":key", key);
    q.bindValue(":val", value);
    q.bindValue(":chunk", chunkId > 0 ? QVariant(static_cast<qint64>(chunkId)) : QVariant());
    q.bindValue(":page", pageNumber);
    return q.exec();
}

QHash<QString, FactEntry> Database::getFactsForJob(int jobId) {
    QHash<QString, FactEntry> result;
    QSqlQuery q(getDb());
    q.prepare("SELECT metric_key, metric_value, source_chunk_id, page_number FROM job_facts WHERE job_id = :job;");
    q.bindValue(":job", jobId);
    if (q.exec()) {
        while (q.next()) {
            FactEntry entry;
            entry.key = q.value(0).toString();
            entry.value = q.value(1).toString();
            entry.sourceChunkId = q.value(2).isNull() ? CoreConstants::NoSourceChunkId : q.value(2).toLongLong();
            entry.pageNumber = q.value(3).toInt();
            result.insert(entry.key, entry);
        }
    }
    return result;
}

quint64 Database::diskSizeBytes() const {
    QFileInfo dbInfo(_dbPath);
    quint64 size = dbInfo.exists() ? static_cast<quint64>(dbInfo.size()) : 0;
    QFileInfo walInfo(_dbPath + "-wal");
    if (walInfo.exists()) size += static_cast<quint64>(walInfo.size());
    QFileInfo shmInfo(_dbPath + "-shm");
    if (shmInfo.exists()) size += static_cast<quint64>(shmInfo.size());
    return size;
}

int Database::totalChunkCount() {
    QSqlQuery q(getDb());
    if (q.exec("SELECT COUNT(*) FROM document_chunks;") && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

bool Database::setExtractionStatus(int jobId, const QString& status) {
    QSqlQuery q(getDb());
    q.prepare("UPDATE ingestion_jobs SET extraction_status = :st WHERE id = :id;");
    q.bindValue(":st", status);
    q.bindValue(":id", jobId);
    return q.exec();
}

QString Database::getExtractionStatus(int jobId) {
    QSqlQuery q(getDb());
    q.prepare("SELECT extraction_status FROM ingestion_jobs WHERE id = :id;");
    q.bindValue(":id", jobId);
    if (q.exec() && q.next()) {
        return q.value(0).toString();
    }
    return QString();
}

bool Database::upsertReportSection(int jobId, const QString& groupId, const QString& dataJson,
                                  const QString& status, const QString& sourceChunkIds) {
    QSqlQuery q(getDb());
    q.prepare("INSERT INTO report_sections (job_id, group_id, data_json, status, source_chunk_ids, updated_at) "
              "VALUES (:job, :group, :data, :status, :chunks, CURRENT_TIMESTAMP) "
              "ON CONFLICT(job_id, group_id) DO UPDATE SET "
              "data_json = excluded.data_json, status = excluded.status, "
              "source_chunk_ids = CASE WHEN excluded.source_chunk_ids IS NOT NULL AND excluded.source_chunk_ids != '' "
              "THEN excluded.source_chunk_ids ELSE report_sections.source_chunk_ids END, "
              "updated_at = CURRENT_TIMESTAMP;");
    q.bindValue(":job", jobId);
    q.bindValue(":group", groupId);
    q.bindValue(":data", dataJson);
    q.bindValue(":status", status);
    q.bindValue(":chunks", sourceChunkIds);
    return q.exec();
}

QVector<ReportSection> Database::getReportSections(int jobId) {
    QSqlQuery q(getDb());
    q.prepare("SELECT group_id, data_json, status, source_chunk_ids FROM report_sections WHERE job_id = :job;");
    q.bindValue(":job", jobId);

    QVector<ReportSection> sections;
    if (q.exec()) {
        while (q.next()) {
            sections.append({q.value(0).toString(), q.value(1).toString(), q.value(2).toString(), q.value(3).toString()});
        }
    }
    return sections;
}

bool Database::lockReport(int jobId) {
    QSqlQuery q(getDb());
    q.prepare("UPDATE ingestion_jobs SET extraction_status = 'ACCEPTED', report_locked_at = CURRENT_TIMESTAMP WHERE id = :id;");
    q.bindValue(":id", jobId);
    return q.exec();
}

bool Database::isReportLocked(int jobId) {
    QSqlQuery q(getDb());
    q.prepare("SELECT report_locked_at FROM ingestion_jobs WHERE id = :id;");
    q.bindValue(":id", jobId);
    if (q.exec() && q.next()) {
        return !q.value(0).isNull();
    }
    return false;
}

QString Database::getJobStatus(int jobId) {
    QSqlQuery q(getDb());
    q.prepare("SELECT status FROM ingestion_jobs WHERE id = :id;");
    q.bindValue(":id", jobId);
    if (q.exec() && q.next()) {
        return q.value(0).toString();
    }
    return QString();
}
int Database::getPendingJobCount() {
    QSqlQuery q(getDb());
    if (q.exec("SELECT COUNT(*) FROM ingestion_jobs WHERE status IN ('PENDING', 'PROCESSING');") && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

bool Database::loadEmbeddingsForJob(int jobId, const std::function<void(int64_t id, const std::vector<float>& vec)>& consumer) {
    QSqlQuery q(getDb());
    q.prepare("SELECT id, embedding FROM document_chunks WHERE job_id = :job AND embedding IS NOT NULL ORDER BY id ASC;");
    q.bindValue(":job", jobId);
    if (!q.exec()) {
        _log.error(QString("Failed to query embeddings for job %1: %2").arg(jobId).arg(q.lastError().text()));
        return false;
    }
    int loaded = 0;
    while (q.next()) {
        int64_t id = q.value(0).toLongLong();
        QByteArray blob = q.value(1).toByteArray();
        if (blob.size() % sizeof(float) == 0 && blob.size() > 0) {
            size_t count = blob.size() / sizeof(float);
            const float* raw = reinterpret_cast<const float*>(blob.constData());
            std::vector<float> vec(raw, raw + count);
            if (consumer) {
                consumer(id, vec);
            }
            ++loaded;
        }
    }
    _log.info(QString("Replayed %1 persisted embedding(s) into index for job %2").arg(loaded).arg(jobId));
    return true;
}

QMap<int64_t, QString> Database::getChunksByIds(const std::vector<int64_t>& ids) {
    QMap<int64_t, QString> result;
    if (ids.empty()) return result;

    QStringList placeholders;
    for (size_t i = 0; i < ids.size(); ++i) {
        placeholders.append("?");
    }

    QSqlQuery q(getDb());
    q.prepare(QString("SELECT id, chunk_text FROM document_chunks WHERE id IN (%1);")
              .arg(placeholders.join(',')));
    for (size_t i = 0; i < ids.size(); ++i) {
        q.addBindValue(static_cast<qint64>(ids[i]));
    }

    if (q.exec()) {
        while (q.next()) {
            result.insert(q.value(0).toLongLong(), q.value(1).toString());
        }
    } else {
        _log.error(QString("Failed to fetch batched chunks: %1").arg(q.lastError().text()));
    }
    return result;
}

QVector<int64_t> Database::getDocumentNeighborChunkIds(int jobId, const QVector<int64_t>& anchorIds) {
    QVector<int64_t> neighbors;
    if (jobId <= 0 || anchorIds.isEmpty()) {
        return neighbors;
    }

    QStringList placeholders;
    for (int i = 0; i < anchorIds.size(); ++i) {
        placeholders.append("?");
    }

    QString sql = QString(
        "WITH ordered_chunks AS ("
        "    SELECT id, document_id, "
        "           LAG(id, 1) OVER (PARTITION BY document_id ORDER BY id ASC) AS prev_id, "
        "           LEAD(id, 1) OVER (PARTITION BY document_id ORDER BY id ASC) AS next_id "
        "    FROM document_chunks "
        "    WHERE job_id = ? "
        ") "
        "SELECT id, prev_id, next_id "
        "FROM ordered_chunks "
        "WHERE id IN (%1);"
    ).arg(placeholders.join(','));

    QSqlQuery q(getDb());
    q.prepare(sql);
    q.addBindValue(jobId);
    for (int64_t aid : anchorIds) {
        q.addBindValue(static_cast<qint64>(aid));
    }

    if (q.exec()) {
        while (q.next()) {
            qint64 currentId = q.value(0).toLongLong();
            QVariant prevVal = q.value(1);
            QVariant nextVal = q.value(2);

            if (!prevVal.isNull()) {
                neighbors.append(prevVal.toLongLong());
            }
            neighbors.append(currentId);
            if (!nextVal.isNull()) {
                neighbors.append(nextVal.toLongLong());
            }
        }
    } else {
        _log.error(QString("Failed to query document neighbor chunks for job %1: %2")
                       .arg(jobId).arg(q.lastError().text()));
    }

    return neighbors;
}

bool Database::deleteJob(int jobId) {
    QSqlQuery q(getDb());
    q.prepare("DELETE FROM ingestion_jobs WHERE id = :id;");
    q.bindValue(":id", jobId);
    bool ok = q.exec();
    if (ok) {
        _log.info(QString("Deleted job %1 (cascading documents, chunks, and sections)").arg(jobId));
    } else {
        _log.error(QString("Failed to delete job %1: %2").arg(jobId).arg(q.lastError().text()));
    }
    return ok;
}

QVector<ReportSummary> Database::getAllReports() {
    QSqlQuery q(getDb());
    q.prepare("SELECT id, status, extraction_status, created_at FROM ingestion_jobs ORDER BY id DESC;");
    QVector<ReportSummary> list;
    if (!q.exec()) {
        _log.error(QString("Failed to query reports: %1").arg(q.lastError().text()));
        return list;
    }

    // 1. Batch fetch all documents for all jobs in a single query
    QHash<int, QStringList> docsByJob;
    QSqlQuery docQ(getDb());
    if (docQ.exec("SELECT job_id, file_path FROM job_documents ORDER BY job_id DESC, document_index ASC;")) {
        while (docQ.next()) {
            docsByJob[docQ.value(0).toInt()].append(docQ.value(1).toString());
        }
    }

    // 2. Batch fetch entity names from job_facts
    QHash<int, QString> entityByJob;
    QSqlQuery factQ(getDb());
    if (factQ.exec("SELECT job_id, metric_value FROM job_facts WHERE metric_key = 'entity_name';")) {
        while (factQ.next()) {
            QString val = factQ.value(1).toString().trimmed();
            if (!val.isEmpty() && val != "null" && val != "N/A") {
                entityByJob.insert(factQ.value(0).toInt(), val);
            }
        }
    }

    // 3. Batch fetch fallback corporate section JSON for jobs that still need title derivation
    QHash<int, QString> sectionEntityByJob;
    QSqlQuery secQ(getDb());
    if (secQ.exec("SELECT job_id, data_json FROM report_sections WHERE group_id IN ('section_a_corporate', 'section_a_general', 'entity_details');")) {
        while (secQ.next()) {
            int jid = secQ.value(0).toInt();
            if (!entityByJob.contains(jid) && !sectionEntityByJob.contains(jid)) {
                QString jsonStr = secQ.value(1).toString();
                auto doc = QJsonDocument::fromJson(jsonStr.toUtf8());
                if (doc.isObject()) {
                    QString entity = doc.object().value("entity_name").toString();
                    if (entity.isEmpty()) entity = doc.object().value("general_company_name").toString();
                    if (!entity.isEmpty() && entity != "null" && entity != "N/A") {
                        sectionEntityByJob.insert(jid, entity.trimmed());
                    }
                }
            }
        }
    }

    // 4. Assemble report summaries in memory
    while (q.next()) {
        ReportSummary summary;
        summary.jobId = q.value(0).toInt();
        summary.status = q.value(1).toString();
        summary.extractionStatus = q.value(2).toString();
        summary.createdAt = q.value(3).toString();
        summary.documents = docsByJob.value(summary.jobId);

        QString derivedTitle = entityByJob.value(summary.jobId);
        if (derivedTitle.isEmpty()) {
            derivedTitle = sectionEntityByJob.value(summary.jobId);
        }
        if (derivedTitle.isEmpty() && !summary.documents.isEmpty()) {
            QString firstDoc = summary.documents.first();
            firstDoc.replace('\\', '/');
            QString base = firstDoc.section('/', -1);
            if (base.contains('.')) base = base.section('.', 0, -2);
            base.replace('_', ' ');
            base.replace('-', ' ');
            derivedTitle = base;
        }
        if (derivedTitle.isEmpty()) {
            derivedTitle = QString("Report #%1").arg(summary.jobId);
        }
        summary.title = derivedTitle;
        list.append(summary);
    }
    return list;
}

QVector<DocumentInfo> Database::getJobDocuments(int jobId) {
    QVector<DocumentInfo> docs;
    QSqlQuery q(getDb());
    q.prepare(
        "SELECT jd.id, jd.file_path, COALESCE(jd.status, 'QUEUED') AS doc_status, "
        "       COALESCE(COUNT(dc.id), 0) AS chunk_cnt, "
        "       COALESCE(MAX(dc.page_num), 0) AS max_page "
        "FROM job_documents jd "
        "LEFT JOIN document_chunks dc ON dc.document_id = jd.id "
        "WHERE jd.job_id = :job "
        "GROUP BY jd.id, jd.file_path, jd.status, jd.document_index "
        "ORDER BY jd.document_index ASC;"
    );
    q.bindValue(":job", jobId);
    if (q.exec()) {
        while (q.next()) {
            DocumentInfo doc;
            doc.id = q.value(0).toInt();
            doc.jobId = jobId;
            doc.filePath = q.value(1).toString();
            doc.status = q.value(2).toString();
            QFileInfo fi(doc.filePath);
            doc.fileName = fi.fileName().isEmpty() ? doc.filePath : fi.fileName();
            doc.chunkCount = q.value(3).toInt();
            doc.pageCount = q.value(4).toInt();
            docs.append(doc);
        }
    } else {
        _log.error(QString("Failed to fetch documents for job %1: %2").arg(jobId).arg(q.lastError().text()));
    }
    return docs;
}

