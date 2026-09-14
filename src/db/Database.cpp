#include "Database.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <iostream>

Database::Database(const QString& dbPath) : _dbPath(dbPath) {}

Database::~Database() {
    if (QSqlDatabase::contains("esg_db_conn")) {
        QSqlDatabase::removeDatabase("esg_db_conn");
    }
}

bool Database::init() {
    auto db = QSqlDatabase::addDatabase("QSQLITE", "esg_db_conn");
    db.setDatabaseName(_dbPath);

    if (!db.open()) {
        std::cerr << "SQLite connection failed: " << db.lastError().text().toStdString() << "\n";
        return false;
    }

    return runMigrations();
}

bool Database::runMigrations() {
    QSqlQuery query(QSqlDatabase::database("esg_db_conn"));
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
                std::cerr << "Migration failed at version " << m.version << ": "
                          << query.lastError().text().toStdString() << "\n";
                return false;
            }
            query.exec(QString("PRAGMA user_version = %1;").arg(m.version));
        }
    }
    return true;
}

bool Database::enqueueJob(const QString& filePath) {
    QSqlQuery q(QSqlDatabase::database("esg_db_conn"));
    q.prepare("INSERT INTO ingestion_jobs (file_path, status) VALUES (:path, 'PENDING');");
    q.bindValue(":path", filePath);
    return q.exec();
}

bool Database::getNextPendingJob(int& outJobId, QString& outFilePath) {
    QSqlQuery q(QSqlDatabase::database("esg_db_conn"));
    if (q.exec("SELECT id, file_path FROM ingestion_jobs WHERE status = 'PENDING' ORDER BY id ASC LIMIT 1;") && q.next()) {
        outJobId = q.value(0).toInt();
        outFilePath = q.value(1).toString();
        return true;
    }
    return false;
}

bool Database::updateJobStatus(int jobId, const QString& status, const QString& error) {
    QSqlQuery q(QSqlDatabase::database("esg_db_conn"));
    q.prepare("UPDATE ingestion_jobs SET status = :st, error_message = :err WHERE id = :id;");
    q.bindValue(":st", status);
    q.bindValue(":err", error);
    q.bindValue(":id", jobId);
    return q.exec();
}

bool Database::insertChunk(int jobId, int pageNum, const QString& chunkText, int64_t vectorId) {
    QSqlQuery q(QSqlDatabase::database("esg_db_conn"));
    q.prepare("INSERT INTO document_chunks (job_id, page_num, chunk_text, vector_id) VALUES (:job, :page, :text, :vec);");
    q.bindValue(":job", jobId);
    q.bindValue(":page", pageNum);
    q.bindValue(":text", chunkText);
    q.bindValue(":vec", static_cast<qint64>(vectorId));
    return q.exec();
}