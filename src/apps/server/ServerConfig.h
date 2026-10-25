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
    // Check user Documents/CarbonESG/models folder
    QString docDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/CarbonESG/models";
    QString docModelPath = QDir(docDir).filePath(QFileInfo(fallback).fileName());
    if (QFile::exists(docModelPath)) {
        return docModelPath;
    }
    if (QFile::exists(fallback)) {
        return fallback;
    }
    QString appRelative = QDir(QCoreApplication::applicationDirPath()).filePath(fallback);
    if (QFile::exists(appRelative)) {
        return appRelative;
    }
    // Check installation root relative path (e.g. <InstallDir>/models/ when exe is in <InstallDir>/bin)
    QString installRootRelative = QDir(QCoreApplication::applicationDirPath() + "/..").filePath(fallback);
    if (QFile::exists(installRootRelative)) {
        return installRootRelative;
    }
    QString parentRelative = QDir(QCoreApplication::applicationDirPath() + "/../../..").filePath(fallback);
    if (QFile::exists(parentRelative)) {
        return parentRelative;
    }
    return fallback;
}

inline QString resolveDatabasePath()
{
    QString envVal = QProcessEnvironment::systemEnvironment().value("CARBON_ENGINE_DB_PATH");
    if (!envVal.isEmpty()) {
        return envVal;
    }
    QString docDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/CarbonESG/data";
    QDir().mkpath(docDir);
    return docDir + "/ce-database.sqlite";
}

inline QString defaultGenModel() {
    if (QFile::exists(Models::LocalQwenFile)) return Models::LocalQwenFile;
    if (QFile::exists(Models::LocalGemmaFile)) return Models::LocalGemmaFile;
    return Models::LocalQwenFile;
}

inline const QString& getDatabaseFilePath()
{
    static const QString s_dbPath = resolveDatabasePath();
    return s_dbPath;
}

inline const QString& getGenerationModelPath()
{
    static const QString s_genPath = resolveModelPath("CARBON_ENGINE_GEN_MODEL", defaultGenModel());
    return s_genPath;
}

inline const QString& getEmbeddingModelPath()
{
    static const QString s_embdPath = resolveModelPath("CARBON_ENGINE_EMBD_MODEL", Models::LocalBgeEmbeddingFile);
    return s_embdPath;
}

#define kDatabaseFilePath getDatabaseFilePath()
#define kGenerationModelPath getGenerationModelPath()
#define kEmbeddingModelPath getEmbeddingModelPath()

#endif // SERVER_CONFIG_H
