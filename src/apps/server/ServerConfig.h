#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <QProcessEnvironment>
#include <QString>

// Dimension for the default bge-small-en-v1.5 embedding model.
const int kEmbeddingDim = 384;
const quint16 kDefaultPort = 8080;

const QString kDatabaseFilePath = "ce-database";
const QString kDefaultGenerationModel = "models/qwen2.5-7b-instruct-q4_k_m.gguf";
const QString kDefaultEmbeddingModel = "models/bge-small-en-v1.5-f16.gguf";

inline QString fromEnvironment(const QString &name, const QString &fallback)
{
    return QProcessEnvironment::systemEnvironment().value(name, fallback);
}

const QString kGenerationModelPath = fromEnvironment("CARBON_ENGINE_GEN_MODEL", kDefaultGenerationModel);
const QString kEmbeddingModelPath = fromEnvironment("CARBON_ENGINE_EMBD_MODEL", kDefaultEmbeddingModel);

#endif // SERVER_CONFIG_H
