#include "Logger.h"

#include <map>
#include <memory>
#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>

namespace {

// Decides where log files live: a `log/` directory next to the running
// executable (dev tree / normal install) when writable, otherwise falls back
// to a per-app directory under the system temp path (e.g. installed to a
// read-only Program Files location).
QString resolveLogDirectory() {
    const QString appRelative = QFileInfo(QCoreApplication::applicationDirPath() + "/log").absoluteFilePath();
    if (QDir().mkpath(appRelative)) {
        QFileInfo probe(appRelative + "/.write-test");
        QFile test(probe.filePath());
        if (test.open(QIODevice::WriteOnly)) {
            test.close();
            test.remove();
            return appRelative;
        }
    }

    const QString fallback = QDir::tempPath() + "/carbon-engine/log";
    QDir().mkpath(fallback);
    return fallback;
}

// One append-only file handle per module/date, shared by every Logger
// instance for that module so lines interleave correctly instead of each
// instance clobbering the file.
class FileLogSink {
public:
    explicit FileLogSink(const QString& filePath) : _file(filePath) {
        QDir().mkpath(QFileInfo(filePath).absolutePath());
        _file.open(QIODevice::Append | QIODevice::Text);
        _stream = std::make_unique<QTextStream>(&_file);
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

QMutex& registryMutex() {
    static QMutex mutex;
    return mutex;
}

std::map<QString, std::shared_ptr<FileLogSink>>& registry() {
    static std::map<QString, std::shared_ptr<FileLogSink>> sinks;
    return sinks;
}

std::shared_ptr<FileLogSink> sinkFor(const QString& moduleName) {
    const QString dateStamp = QDate::currentDate().toString("yyyy-MM-dd");
    const QString key = moduleName + "_" + dateStamp;

    QMutexLocker locker(&registryMutex());
    auto it = registry().find(key);
    if (it != registry().end()) {
        return it->second;
    }

    const QString filePath = resolveLogDirectory() + "/" + key + ".log";
    auto sink = std::make_shared<FileLogSink>(filePath);
    registry().emplace(key, sink);
    return sink;
}

} // namespace

Logger::Logger(const QString& moduleName) : _moduleName(moduleName) {}

void Logger::debug(const QString& message) const { log(LogLevel::Debug, message); }
void Logger::info(const QString& message) const { log(LogLevel::Info, message); }
void Logger::warn(const QString& message) const { log(LogLevel::Warning, message); }
void Logger::warning(const QString& message) const { log(LogLevel::Warning, message); }
void Logger::error(const QString& message) const { log(LogLevel::Error, message); }
void Logger::critical(const QString& message) const { log(LogLevel::Critical, message); }

void Logger::log(LogLevel level, const QString& message) const {
    sinkFor(_moduleName)->write(level, _moduleName, message);
}
