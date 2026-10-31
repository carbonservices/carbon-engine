#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QString>

#include "constants/CommonConstants.h"

const int kEmbeddingDim = ServerDefaults::DefaultEmbeddingDim;
const quint16 kDefaultPort = ServerDefaults::DefaultPort;

inline QString resolveModelPath(const QString &fileName)
{
    QString baseName = QFileInfo(fileName).fileName();
    QString docDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Carbon ESG/models";
    QDir().mkpath(docDir);
    return QDir(docDir).filePath(baseName);
}

inline QString resolveDatabasePath()
{
    QString docDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Carbon ESG/data";
    QDir().mkpath(docDir);
    return docDir + "/ce-database.sqlite";
}

inline const QString& getDatabaseFilePath()
{
    static const QString s_dbPath = resolveDatabasePath();
    return s_dbPath;
}

inline QString getGenerationModelPath()
{
    return resolveModelPath(Models::LocalGemmaFile);
}

inline QString getEmbeddingModelPath()
{
    return resolveModelPath(Models::LocalBgeEmbeddingFile);
}

#define kDatabaseFilePath getDatabaseFilePath()
#define kGenerationModelPath getGenerationModelPath()
#define kEmbeddingModelPath getEmbeddingModelPath()

#endif // SERVER_CONFIG_H
