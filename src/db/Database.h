#pragma once
#include <QString>
#include <QSqlDatabase>
#include <vector>

class Database {
public:
    explicit Database(const QString& dbPath);
    ~Database();

    bool init();
    bool enqueueJob(const QString& filePath);
    bool getNextPendingJob(int& outJobId, QString& outFilePath);
    bool updateJobStatus(int jobId, const QString& status, const QString& error = "");
    bool insertChunk(int jobId, int pageNum, const QString& chunkText, int64_t vectorId);

private:
    QString _dbPath;
    bool runMigrations();
};