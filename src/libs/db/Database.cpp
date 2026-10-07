#include "Database.h"

// Qt
#include <QSqlQuery>
#include <QSqlError>
#include <QStringList>
#include <QVariant>

Database::Database(const QString& dbPath) : _dbPath(dbPath) {}

Database::~Database() {}

QSqlDatabase Database::getDb() {
    // Use the memory address of the current thread ID as a unique connection name
    QString connName = QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    
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
    return runMigrations();
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
    query.exec("PRAGMA user_version;");
    int currentVersion = 0;
    if (query.next()) {
        currentVersion = query.value(0).toInt();
    }

    struct Migration {
        int version;
        const char* sql;
    };

    const std::vector<Migration> migrations = {
        {1, "CREATE TABLE IF NOT EXISTS ingestion_jobs ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "file_path TEXT NOT NULL, "
            "status TEXT CHECK(status IN ('PENDING', 'PROCESSING', 'COMPLETED', 'FAILED')) DEFAULT 'PENDING', "
            "error_message TEXT, "
            "created_at DATETIME DEFAULT CURRENT_TIMESTAMP);"},
        {2, "CREATE TABLE IF NOT EXISTS document_chunks ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "job_id INTEGER, "
            "page_num INTEGER, "
            "chunk_text TEXT, "
            "vector_id INTEGER, "
            "FOREIGN KEY(job_id) REFERENCES ingestion_jobs(id));"},
        {3, "CREATE INDEX IF NOT EXISTS idx_document_chunks_vector_id ON document_chunks(vector_id);"
            "CREATE INDEX IF NOT EXISTS idx_document_chunks_job_id ON document_chunks(job_id);"
            "CREATE INDEX IF NOT EXISTS idx_ingestion_jobs_status ON ingestion_jobs(status);"
            // SQLite can't ALTER a FOREIGN KEY clause in place, so the cascade is added by
            // rebuilding document_chunks and copying its rows across.
            "CREATE TABLE document_chunks_new ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "job_id INTEGER, "
            "page_num INTEGER, "
            "chunk_text TEXT, "
            "vector_id INTEGER, "
            "FOREIGN KEY(job_id) REFERENCES ingestion_jobs(id) ON DELETE CASCADE);"
            "INSERT INTO document_chunks_new (id, job_id, page_num, chunk_text, vector_id) "
            "SELECT id, job_id, page_num, chunk_text, vector_id FROM document_chunks;"
            "DROP TABLE document_chunks;"
            "ALTER TABLE document_chunks_new RENAME TO document_chunks;"
            "CREATE INDEX IF NOT EXISTS idx_document_chunks_vector_id ON document_chunks(vector_id);"
            "CREATE INDEX IF NOT EXISTS idx_document_chunks_job_id ON document_chunks(job_id);"}
    };

    for (const auto& m : migrations) {
        if (m.version > currentVersion) {
            QStringList statements = QString(m.sql).split(';', Qt::SkipEmptyParts);
            bool migrationOk = true;
            for (const QString& statement : statements) {
                const QString trimmed = statement.trimmed();
                if (trimmed.isEmpty())
                    continue;
                if (!query.exec(trimmed)) {
                    _log.error(QString("Migration failed at version %1: %2")
                        .arg(m.version).arg(query.lastError().text()));
                    migrationOk = false;
                    break;
                }
            }
            if (!migrationOk)
                return false;
            query.exec(QString("PRAGMA user_version = %1;").arg(m.version));
        }
    }
    return true;
}

int Database::enqueueJob(const QString& filePath) {
    QSqlQuery q(getDb());
    q.prepare("INSERT INTO ingestion_jobs (file_path, status) VALUES (:path, 'PENDING');");
    q.bindValue(":path", filePath);
    if (!q.exec()) return -1;
    return q.lastInsertId().toInt();
}

bool Database::getNextPendingJob(int& outJobId, QString& outFilePath) {
    QSqlQuery q(getDb());
    if (q.exec("SELECT id, file_path FROM ingestion_jobs WHERE status = 'PENDING' ORDER BY id ASC LIMIT 1;") && q.next()) {
        outJobId = q.value(0).toInt();
        outFilePath = q.value(1).toString();
        return true;
    }
    return false;
}

bool Database::updateJobStatus(int jobId, const QString& status, const QString& error) {
    QSqlQuery q(getDb());
    q.prepare("UPDATE ingestion_jobs SET status = :st, error_message = :err WHERE id = :id;");
    q.bindValue(":st", status);
    q.bindValue(":err", error);
    q.bindValue(":id", jobId);
    return q.exec();
}

bool Database::insertChunk(int jobId, int pageNum, const QString& chunkText, int64_t vectorId) {
    QSqlQuery q(getDb());
    q.prepare("INSERT INTO document_chunks (job_id, page_num, chunk_text, vector_id) VALUES (:job, :page, :text, :vec);");
    q.bindValue(":job", jobId);
    q.bindValue(":page", pageNum);
    q.bindValue(":text", chunkText);
    q.bindValue(":vec", static_cast<qint64>(vectorId));
    return q.exec();
}

QString Database::getChunkByVectorId(int64_t vectorId) {
    QSqlQuery q(getDb());
    q.prepare("SELECT chunk_text FROM document_chunks WHERE vector_id = :vec;");
    q.bindValue(":vec", static_cast<qint64>(vectorId));
    if (q.exec() && q.next()) {
        return q.value(0).toString();
    }
    return "";
}

QVector<QString> Database::getChunksForJob(int jobId) {
    QSqlQuery q(getDb());
    q.prepare("SELECT chunk_text FROM document_chunks WHERE job_id = :job ORDER BY page_num ASC;");
    q.bindValue(":job", jobId);

    QVector<QString> chunks;
    if (q.exec()) {
        while (q.next()) {
            chunks.append(q.value(0).toString());
        }
    }
    return chunks;
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
