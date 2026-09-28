#ifndef DATABASE_H
#define DATABASE_H

#include <QString>
#include <QSqlDatabase>
#include <QVector>
#include <QThread>
#include "logging/Logger.h"

class Database {
public:
    explicit Database(const QString& dbPath);
    ~Database();

    bool init();
    int enqueueJob(const QString& filePath);
    bool getNextPendingJob(int& outJobId, QString& outFilePath);
    bool updateJobStatus(int jobId, const QString& status, const QString& error = "");
    bool insertChunk(int jobId, int pageNum, const QString& chunkText, int64_t vectorId);
    QVector<QString> getChunksForJob(int jobId);
    QString getJobStatus(int jobId);

private:
    QString _dbPath;
    Logger _log{"Database"};
    bool runMigrations();
    
    // Helper to guarantee a valid connection for the calling thread
    QSqlDatabase getDb(); 
};

#endif // DATABASE_H