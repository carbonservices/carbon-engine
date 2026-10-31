#pragma once

#include <QElapsedTimer>
#include <QFile>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include <QUrl>

#include "logging/Logger.h"

struct ModelDownloadItem {
    QString fileName;
    QString displayName;
    QString url;
    qint64 estimatedBytes;
    QString sha256;
};

class ModelDownloader : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isDownloading READ isDownloading NOTIFY downloadingChanged)
    Q_PROPERTY(bool modelsReady READ areModelsPresent NOTIFY modelsReadyChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QString currentModelName READ currentModelName NOTIFY currentModelChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(int progressPercent READ progressPercent NOTIFY progressChanged)
    Q_PROPERTY(QString downloadSpeed READ downloadSpeed NOTIFY progressChanged)
    Q_PROPERTY(QString targetDirectory READ targetDirectory CONSTANT)

public:
    explicit ModelDownloader(QObject* parent = nullptr);
    ~ModelDownloader() override;

    static QString getModelsDirectory();
    static bool areModelsPresent();
    static bool isModelPresent(const QString& fileName);
    static QString formattedTotalSize();

    bool isDownloading() const { return _isDownloading; }
    bool hasError() const { return _hasError; }
    QString errorMessage() const { return _errorMessage; }
    QString statusText() const { return _statusText; }
    QString currentModelName() const;
    qreal progress() const { return _progress; }
    int progressPercent() const { return static_cast<int>(_progress * 100.0); }
    QString downloadSpeed() const { return _speedString; }
    QString targetDirectory() const;

    Q_INVOKABLE void startDownload();
    Q_INVOKABLE void cancelDownload();
    Q_INVOKABLE void retryDownload();
    Q_INVOKABLE bool checkModels();

signals:
    void downloadingChanged();
    void modelsReadyChanged();
    void errorChanged();
    void statusChanged();
    void currentModelChanged();
    void progressChanged();
    void downloadCompleted();

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onReadyRead();
    void onDownloadFinished();
    void onNetworkError(QNetworkReply::NetworkError code);
    void onSslErrors(const QList<QSslError>& errors);

private:
    void startNextItem();
    void finishAllDownloads();
    void setError(const QString& msg);
    void updateSpeed();
    void buildQueue();

    QNetworkAccessManager* _nam = nullptr;
    QNetworkReply* _currentReply = nullptr;
    QFile _outputFile;
    QString _partFilePath;
    QString _finalFilePath;

    QList<ModelDownloadItem> _queue;
    int _currentIndex = 0;

    qint64 _currentFileBytesReceived = 0;
    qint64 _currentFileBytesTotal = 0;
    qint64 _completedBytes = 0;
    qint64 _totalBatchBytes = 0;

    QElapsedTimer _speedTimer;
    qint64 _speedSampleTime = 0;
    qint64 _speedSampleBytes = 0;
    QString _speedString;

    qreal _progress = 0.0;
    bool _isDownloading = false;
    bool _hasError = false;
    QString _errorMessage;
    QString _statusText;

    Logger _log;
};
