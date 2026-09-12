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

private:
    QString m_dbPath;
    bool runMigrations();
};