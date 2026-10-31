#pragma once

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QMap>
#include <cstdint>

class ZipReader {
public:
    explicit ZipReader(const QString &filePath);
    ~ZipReader();

    bool isOpen() const;
    QStringList fileList() const;
    bool hasFile(const QString &fileName) const;
    QByteArray extractFile(const QString &fileName) const;

private:
    struct Entry {
        uint16_t compressionMethod{0};
        uint32_t compressedSize{0};
        uint32_t uncompressedSize{0};
        uint32_t localHeaderOffset{0};
        uint32_t crc32{0};
    };

    bool parseCentralDirectory();

    QString m_filePath;
    bool m_isOpen{false};
    QMap<QString, Entry> m_entries;
    QStringList m_fileList;
};

