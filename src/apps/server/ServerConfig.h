#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QString>

#include "constants/CommonConstants.h"

const int kEmbeddingDim = ServerDefaults::DefaultEmbeddingDim;
const quint16 kDefaultPort = ServerDefaults::DefaultPort;

inline QString resolveModelPath(const QString &name, const QString &fallback)
{
    QString envVal = QProcessEnvironment::systemEnvironment().value(name);
    if (!envVal.isEmpty()) {
        return envVal;
    }
    QString docDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Carbon ESG/models";
    return QDir(docDir).filePath(QFileInfo(fallback).fileName());
}

inline QString resolveDatabasePath()
{
    QString envVal = QProcessEnvironment::systemEnvironment().value("CARBON_ENGINE_DB_PATH");
    if (!envVal.isEmpty()) {
        return envVal;
    }
    QString docDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Carbon ESG/data";
    QDir().mkpath(docDir);
    return docDir + "/ce-database.sqlite";
}

inline QString defaultGenModel() {
    return Models::LocalGemmaFile;
}

inline const QString& getDatabaseFilePath()
{
    static const QString s_dbPath = resolveDatabasePath();
    return s_dbPath;
}

inline QString getGenerationModelPath()
{
    return resolveModelPath("CARBON_ENGINE_GEN_MODEL", defaultGenModel());
}

inline QString getEmbeddingModelPath()
{
    return resolveModelPath("CARBON_ENGINE_EMBD_MODEL", Models::LocalBgeEmbeddingFile);
}

#define kDatabaseFilePath getDatabaseFilePath()
#define kGenerationModelPath getGenerationModelPath()
#define kEmbeddingModelPath getEmbeddingModelPath()

#endif // SERVER_CONFIG_H
