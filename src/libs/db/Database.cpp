#include "Database.h"

// Qt
#include <QSqlQuery>
#include <QSqlError>
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
    }
    return db;
}

bool Database::init() {
    return runMigrations();
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
            "FOREIGN KEY(job_id) REFERENCES ingestion_jobs(id));"}
    };

    for (const auto& m : migrations) {
        if (m.version > currentVersion) {
            if (!query.exec(m.sql)) {
                _log.error(QString("Migration failed at version %1: %2")
                    .arg(m.version).arg(query.lastError().text()));
                return false;
            }
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