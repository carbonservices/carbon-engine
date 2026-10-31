#include "ModelDownloader.h"
#include "constants/CommonConstants.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QStandardPaths>

static const ModelDownloadItem kStandardModels[] = {
    {
        "bge-small-en-v1.5-f16.gguf",
        "BGE Small Embedding Model",
        "https://huggingface.co/compendiumlabs/bge-small-en-v1.5-gguf/resolve/main/bge-small-en-v1.5-f16.gguf",
        67308128,
        "f0b2fef971e8366438bfd2d9aefea1b0115919389448806d290237f638bae999"
    },
    {
        "gemma-2-2b-it-q4_k_m.gguf",
        "Gemma 2 2B Instruct LLM",
        "https://huggingface.co/bartowski/gemma-2-2b-it-GGUF/resolve/main/gemma-2-2b-it-Q4_K_M.gguf",
        1708582752,
        "e0aee85060f168f0f2d8473d7ea41ce2f3230c1bc1374847505ea599288a7787"
    }
};

ModelDownloader::ModelDownloader(QObject* parent)
    : QObject(parent)
    , _log("ModelDownloader")
{
    _nam = new QNetworkAccessManager(this);
    initializeTlsBackend();
    checkModels();
}

void ModelDownloader::initializeTlsBackend()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    if (!QCoreApplication::libraryPaths().contains(appDir)) {
        QCoreApplication::addLibraryPath(appDir);
    }
    const QString pluginsDir = appDir + "/plugins";
    if (QDir(pluginsDir).exists() && !QCoreApplication::libraryPaths().contains(pluginsDir)) {
        QCoreApplication::addLibraryPath(pluginsDir);
    }

    const QStringList backends = QSslSocket::availableBackends();
    _log.info(QString("Available TLS backends: [%1]").arg(backends.join(", ")));

#ifdef Q_OS_WIN
    if (backends.contains("schannel", Qt::CaseInsensitive)) {
        if (QSslSocket::setActiveBackend("schannel")) {
            _log.info("Activated native Windows Schannel TLS backend.");
        } else {
            _log.warn("Failed to activate native Windows Schannel TLS backend.");
        }
    }
#endif

    const bool hasSsl = QSslSocket::supportsSsl();
    _log.info(QString("TLS Status: supportsSsl=%1, activeBackend=%2, sslLibraryBuildVersion=%3")
                  .arg(hasSsl ? "true" : "false")
                  .arg(QSslSocket::activeBackend())
                  .arg(QSslSocket::sslLibraryBuildVersionString()));
}

ModelDownloader::~ModelDownloader()
{
    cancelDownload();
}

QString ModelDownloader::getModelsDirectory()
{
    QString docDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Carbon ESG/models";
    QDir().mkpath(docDir);
    return docDir;
}

bool ModelDownloader::isModelPresent(const QString& fileName)
{
    return QFile::exists(QDir(getModelsDirectory()).filePath(QFileInfo(fileName).fileName()));
}

QString ModelDownloader::formattedTotalSize()
{
    qint64 total = 0;
    for (const ModelDownloadItem& item : kStandardModels) {
        total += item.estimatedBytes;
    }
    double gb = static_cast<double>(total) / (1024.0 * 1024.0 * 1024.0);
    return QString("%1 GB").arg(QString::number(gb, 'f', 2));
}

bool ModelDownloader::areModelsPresent()
{
    for (const ModelDownloadItem& item : kStandardModels) {
        if (!isModelPresent(item.fileName)) {
            return false;
        }
    }
    return true;
}

QString ModelDownloader::targetDirectory() const
{
    return getModelsDirectory();
}

QString ModelDownloader::currentModelName() const
{
    if (_currentIndex >= 0 && _currentIndex < _queue.size()) {
        return _queue[_currentIndex].displayName;
    }
    return QString();
}

bool ModelDownloader::checkModels()
{
    bool ready = areModelsPresent();
    emit modelsReadyChanged();
    return ready;
}

void ModelDownloader::buildQueue()
{
    _queue.clear();
    _totalBatchBytes = 0;
    _completedBytes = 0;

    for (const ModelDownloadItem& item : kStandardModels) {
        if (!isModelPresent(item.fileName)) {
            _queue.append(item);
            _totalBatchBytes += item.estimatedBytes;
        }
    }

    if (_totalBatchBytes <= 0) {
        _totalBatchBytes = 1;
    }
}

void ModelDownloader::startDownload()
{
    if (_isDownloading) {
        return;
    }

    if (!QSslSocket::supportsSsl()) {
        initializeTlsBackend();
    }

    if (!QSslSocket::supportsSsl()) {
        QString err = "TLS initialization failed: No functional TLS backend is available. Please ensure Qt TLS plugins (qschannelbackend.dll) are installed.";
        _log.error(err);
        setError(err);
        return;
    }

    buildQueue();

    if (_queue.isEmpty()) {
        _log.info("All AI models are already present on disk.");
        _hasError = false;
        _errorMessage.clear();
        _statusText = "All AI models are already installed.";
        emit statusChanged();
        emit errorChanged();
        emit modelsReadyChanged();
        emit downloadCompleted();
        return;
    }

    _isDownloading = true;
    _hasError = false;
    _errorMessage.clear();
    _currentIndex = 0;
    _progress = 0.0;
    _speedString.clear();

    _speedTimer.start();
    _speedSampleTime = 0;
    _speedSampleBytes = 0;

    emit downloadingChanged();
    emit errorChanged();
    emit progressChanged();

    startNextItem();
}

void ModelDownloader::cancelDownload()
{
    if (!_isDownloading && !_currentReply) {
        return;
    }

    _log.info("Cancelling active model download.");
    if (_currentReply) {
        _currentReply->abort();
        _currentReply->deleteLater();
        _currentReply = nullptr;
    }

    if (_outputFile.isOpen()) {
        _outputFile.close();
    }
    if (!_partFilePath.isEmpty() && QFile::exists(_partFilePath)) {
        QFile::remove(_partFilePath);
    }

    _isDownloading = false;
    _statusText = "Download cancelled.";
    _speedString.clear();

    emit downloadingChanged();
    emit statusChanged();
    emit progressChanged();
}

void ModelDownloader::retryDownload()
{
    cancelDownload();
    startDownload();
}

void ModelDownloader::startNextItem()
{
    if (_currentIndex >= _queue.size()) {
        finishAllDownloads();
        return;
    }

    const ModelDownloadItem& item = _queue[_currentIndex];
    QString targetDir = getModelsDirectory();
    _finalFilePath = QDir(targetDir).filePath(item.fileName);
    _partFilePath = _finalFilePath + ".part";

    _outputFile.setFileName(_partFilePath);
    if (!_outputFile.open(QIODevice::WriteOnly)) {
        setError(QString("Failed to create destination file: %1").arg(_partFilePath));
        return;
    }

    _currentFileBytesReceived = 0;
    _currentFileBytesTotal = item.estimatedBytes;

    _statusText = QString("Downloading %1 (%2 of %3)...")
                      .arg(item.displayName)
                      .arg(_currentIndex + 1)
                      .arg(_queue.size());
    _log.info(_statusText);

    emit statusChanged();
    emit currentModelChanged();

    QNetworkRequest request(QUrl(item.url));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, "CarbonESG/1.0 (Desktop Client)");

    _currentReply = _nam->get(request);

    connect(_currentReply, &QNetworkReply::downloadProgress, this, &ModelDownloader::onDownloadProgress);
    connect(_currentReply, &QNetworkReply::readyRead, this, &ModelDownloader::onReadyRead);
    connect(_currentReply, &QNetworkReply::finished, this, &ModelDownloader::onDownloadFinished);
    connect(_currentReply, &QNetworkReply::errorOccurred, this, &ModelDownloader::onNetworkError);
    connect(_currentReply, &QNetworkReply::sslErrors, this, &ModelDownloader::onSslErrors);
}

void ModelDownloader::onReadyRead()
{
    if (_currentReply && _outputFile.isOpen()) {
        QByteArray chunk = _currentReply->readAll();
        if (!chunk.isEmpty()) {
            _outputFile.write(chunk);
        }
    }
}

void ModelDownloader::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    _currentFileBytesReceived = bytesReceived;
    if (bytesTotal > 0) {
        _currentFileBytesTotal = bytesTotal;
    }

    qint64 totalDone = _completedBytes + bytesReceived;
    if (_totalBatchBytes > 0) {
        _progress = static_cast<qreal>(totalDone) / static_cast<qreal>(_totalBatchBytes);
        if (_progress > 1.0) {
            _progress = 1.0;
        }
    }

    updateSpeed();
    emit progressChanged();
}

void ModelDownloader::updateSpeed()
{
    qint64 elapsedMs = _speedTimer.elapsed();
    if (elapsedMs - _speedSampleTime >= 750) {
        qint64 totalDone = _completedBytes + _currentFileBytesReceived;
        qint64 deltaBytes = totalDone - _speedSampleBytes;
        qint64 deltaMs = elapsedMs - _speedSampleTime;

        if (deltaMs > 0 && deltaBytes >= 0) {
            double bytesPerSec = (static_cast<double>(deltaBytes) * 1000.0) / static_cast<double>(deltaMs);
            if (bytesPerSec >= (1024.0 * 1024.0)) {
                _speedString = QString("%1 MB/s").arg(QString::number(bytesPerSec / (1024.0 * 1024.0), 'f', 1));
            } else if (bytesPerSec >= 1024.0) {
                _speedString = QString("%1 KB/s").arg(QString::number(bytesPerSec / 1024.0, 'f', 0));
            } else {
                _speedString = QString("%1 B/s").arg(QString::number(bytesPerSec, 'f', 0));
            }
        }

        _speedSampleTime = elapsedMs;
        _speedSampleBytes = totalDone;
    }
}

void ModelDownloader::onDownloadFinished()
{
    if (!_currentReply) {
        return;
    }

    if (_outputFile.isOpen()) {
        _outputFile.flush();
        _outputFile.close();
    }

    if (_currentReply->error() != QNetworkReply::NoError) {
        // Handled in onNetworkError
        return;
    }

    _currentReply->deleteLater();
    _currentReply = nullptr;

    // Cryptographic SHA-256 verification before promoting .part to final
    if (_currentIndex >= 0 && _currentIndex < _queue.size()) {
        const ModelDownloadItem& item = _queue[_currentIndex];
        if (!item.sha256.isEmpty()) {
            _log.info(QString("Verifying SHA-256 integrity for %1...").arg(item.fileName));
            QFile partFile(_partFilePath);
            if (partFile.open(QIODevice::ReadOnly)) {
                QCryptographicHash hasher(QCryptographicHash::Sha256);
                if (hasher.addData(&partFile)) {
                    QString calculated = QString::fromLatin1(hasher.result().toHex()).toLower();
                    if (calculated != item.sha256.toLower()) {
                        partFile.close();
                        QFile::remove(_partFilePath);
                        setError(QString("Cryptographic SHA-256 integrity failure for %1 (expected: %2, got: %3)")
                                     .arg(item.fileName, item.sha256, calculated));
                        return;
                    }
                    _log.info(QString("SHA-256 verification passed for %1 (%2)").arg(item.fileName, calculated));
                }
                partFile.close();
            }
        }
    }

    // Remove old final file if present, then rename .part to final
    if (QFile::exists(_finalFilePath)) {
        QFile::remove(_finalFilePath);
    }

    if (!QFile::rename(_partFilePath, _finalFilePath)) {
        setError(QString("Failed to finalize downloaded model file: %1").arg(_finalFilePath));
        return;
    }

    _log.info(QString("Successfully downloaded: %1").arg(_finalFilePath));
    _completedBytes += _currentFileBytesTotal;

    _currentIndex++;
    startNextItem();
}

void ModelDownloader::finishAllDownloads()
{
    _isDownloading = false;
    _hasError = false;
    _errorMessage.clear();
    _progress = 1.0;
    _speedString.clear();
    _statusText = "All AI models installed and ready.";
    _log.info(_statusText);

    emit downloadingChanged();
    emit progressChanged();
    emit statusChanged();
    emit modelsReadyChanged();
    emit downloadCompleted();
}

void ModelDownloader::onNetworkError(QNetworkReply::NetworkError code)
{
    if (code == QNetworkReply::OperationCanceledError) {
        return;
    }

    QString errStr = _currentReply ? _currentReply->errorString() : "Unknown network error";
    if (code == QNetworkReply::SslHandshakeFailedError || code == 99 /* ProtocolFailure / TLS initialization */) {
        if (!QSslSocket::supportsSsl()) {
            errStr += " (No functional TLS backend found; please verify qschannelbackend.dll in bin/tls)";
        }
    }
    setError(QString("Download failed: %1 (Code %2)").arg(errStr).arg(code));
}

void ModelDownloader::onSslErrors(const QList<QSslError>& errors)
{
    QString details;
    for (const QSslError& err : errors) {
        if (!details.isEmpty()) {
            details += "; ";
        }
        details += err.errorString();
    }
    _log.error(QString("SSL certificate verification error during model download: %1").arg(details));
    setError(QString("SSL certificate verification failed: %1").arg(details));
}

void ModelDownloader::setError(const QString& msg)
{
    _log.error(msg);
    _isDownloading = false;
    _hasError = true;
    _errorMessage = msg;
    _statusText = "Download failed.";

    if (_currentReply) {
        _currentReply->abort();
        _currentReply->deleteLater();
        _currentReply = nullptr;
    }

    if (_outputFile.isOpen()) {
        _outputFile.close();
    }

    emit downloadingChanged();
    emit errorChanged();
    emit statusChanged();
}
