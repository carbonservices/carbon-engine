#include "Logger.h"

#include <map>
#include <memory>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>

static QString s_customLogDir;
static QMutex s_logDirMutex;

QString resolveLogDirectory() {
    QMutexLocker locker(&s_logDirMutex);
    if (!s_customLogDir.isEmpty()) {
        QDir().mkpath(s_customLogDir);
        return s_customLogDir;
    }

    QString docsLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (docsLocation.isEmpty()) {
        docsLocation = QDir::homePath() + "/Documents";
    }
    QString docsLogDir = QDir::cleanPath(docsLocation + "/Carbon ESG/logs");
    QDir().mkpath(docsLogDir);
    return docsLogDir;
}

// One append-only file handle per module/date, shared by every Logger
// instance for that module so lines interleave correctly instead of each
// instance clobbering the file.
class FileLogSink {
public:
    explicit FileLogSink(const QString& filePath) : _file(filePath) {
        QDir().mkpath(QFileInfo(filePath).absolutePath());
        if (_file.open(QIODevice::Append | QIODevice::Text)) {
            _stream = std::make_unique<QTextStream>(&_file);
        }
    }

    ~FileLogSink() {
        if (_file.isOpen()) {
            _stream->flush();
            _file.close();
        }
    }

    void write(LogLevel level, const QString& moduleName, const QString& message) {
        QMutexLocker locker(&_mutex);
        if (!_file.isOpen()) return;

        *_stream << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz")
                  << " [" << toString(level) << "] "
                  << moduleName << ": " << message << "\n";
        _stream->flush();
    }

private:
    QFile _file;
    std::unique_ptr<QTextStream> _stream;
    QMutex _mutex;
};

static QMutex& registryMutex() {
    static QMutex mutex;
    return mutex;
}

static std::map<QString, std::shared_ptr<FileLogSink>>& registry() {
    static std::map<QString, std::shared_ptr<FileLogSink>> sinks;
    return sinks;
}

static std::shared_ptr<FileLogSink> sinkFor(const QString& moduleName) {
    const QString dateStamp = QDate::currentDate().toString("yyyy-MM-dd");
    const QString prefix = moduleName + "_";
    const QString key = prefix + dateStamp;

    QMutexLocker locker(&registryMutex());
    auto& reg = registry();
    auto it = reg.find(key);
    if (it != reg.end()) {
        return it->second;
    }

    // Evict expired daily sinks for this module so old file handles close cleanly
    for (auto iter = reg.begin(); iter != reg.end(); ) {
        if (iter->first.startsWith(prefix)) {
            iter = reg.erase(iter);
        } else {
            ++iter;
        }
    }

    const QString filePath = resolveLogDirectory() + "/" + key + ".log";
    auto sink = std::make_shared<FileLogSink>(filePath);
    reg.emplace(key, sink);
    return sink;
}

Logger::Logger(const QString& moduleName) : _moduleName(moduleName) {}

void Logger::debug(const QString& message) const { log(LogLevel::Debug, message); }
void Logger::info(const QString& message) const { log(LogLevel::Info, message); }
void Logger::warn(const QString& message) const { log(LogLevel::Warning, message); }
void Logger::error(const QString& message) const { log(LogLevel::Error, message); }
void Logger::critical(const QString& message) const { log(LogLevel::Critical, message); }

void Logger::log(LogLevel level, const QString& message) const {
    sinkFor(_moduleName)->write(level, _moduleName, message);
}

void Logger::setLogDirectory(const QString& dir) {
    QMutexLocker locker(&s_logDirMutex);
    s_customLogDir = dir;
}

QString Logger::logDirectory() {
    return resolveLogDirectory();
}
