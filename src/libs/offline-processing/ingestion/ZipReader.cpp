#include "ZipReader.h"

#include <QFile>
#include <QtEndian>
#include <zlib.h>
#include <algorithm>
#include <cstring>

ZipReader::ZipReader(const QString &filePath)
    : m_filePath(filePath)
{
    m_isOpen = parseCentralDirectory();
}

ZipReader::~ZipReader() = default;

bool ZipReader::isOpen() const
{
    return m_isOpen;
}

QStringList ZipReader::fileList() const
{
    return m_fileList;
}

bool ZipReader::hasFile(const QString &fileName) const
{
    return m_entries.contains(fileName);
}

bool ZipReader::parseCentralDirectory()
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const qint64 fileSize = file.size();
    if (fileSize < 22) { // Minimum End of Central Directory (EOCD) size
        return false;
    }

    // Search backwards for EOCD signature (0x06054b50)
    // EOCD may be followed by a variable-length archive comment up to 65535 bytes
    const qint64 maxSearch = std::min(fileSize, static_cast<qint64>(65535 + 22));
    const qint64 searchStart = fileSize - maxSearch;
    if (!file.seek(searchStart)) {
        return false;
    }

    const QByteArray searchBuf = file.read(maxSearch);
    if (searchBuf.size() != maxSearch) {
        return false;
    }

    const char eocdSig[4] = {'P', 'K', 0x05, 0x06};
    qint64 eocdPos = -1;
    for (qint64 i = searchBuf.size() - 22; i >= 0; --i) {
        if (std::memcmp(searchBuf.constData() + i, eocdSig, 4) == 0) {
            eocdPos = i;
            break;
        }
    }

    if (eocdPos < 0) {
        return false;
    }

    const uchar *p = reinterpret_cast<const uchar*>(searchBuf.constData() + eocdPos);
    const uint16_t totalEntries = qFromLittleEndian<uint16_t>(p + 10);
    const uint32_t cdSize = qFromLittleEndian<uint32_t>(p + 12);
    const uint32_t cdOffset = qFromLittleEndian<uint32_t>(p + 16);

    if (static_cast<qint64>(cdOffset) + cdSize > fileSize) {
        return false;
    }

    if (!file.seek(cdOffset)) {
        return false;
    }

    const QByteArray cdBuf = file.read(cdSize);
    if (cdBuf.size() != static_cast<int>(cdSize)) {
        return false;
    }

    const char cdSig[4] = {'P', 'K', 0x01, 0x02};
    const uchar *cdPtr = reinterpret_cast<const uchar*>(cdBuf.constData());
    const uchar *cdEnd = cdPtr + cdBuf.size();

    for (uint16_t i = 0; i < totalEntries && cdPtr + 46 <= cdEnd; ++i) {
        if (std::memcmp(cdPtr, cdSig, 4) != 0) {
            break;
        }

        const uint16_t compMethod = qFromLittleEndian<uint16_t>(cdPtr + 10);
        const uint32_t crc = qFromLittleEndian<uint32_t>(cdPtr + 16);
        const uint32_t compSize = qFromLittleEndian<uint32_t>(cdPtr + 20);
        const uint32_t uncompSize = qFromLittleEndian<uint32_t>(cdPtr + 24);
        const uint16_t fnLen = qFromLittleEndian<uint16_t>(cdPtr + 28);
        const uint16_t extraLen = qFromLittleEndian<uint16_t>(cdPtr + 30);
        const uint16_t commentLen = qFromLittleEndian<uint16_t>(cdPtr + 32);
        const uint32_t localOffset = qFromLittleEndian<uint32_t>(cdPtr + 42);

        if (cdPtr + 46 + fnLen > cdEnd) {
            break;
        }

        const QString fileName = QString::fromUtf8(reinterpret_cast<const char*>(cdPtr + 46), fnLen);
        Entry entry;
        entry.compressionMethod = compMethod;
        entry.crc32 = crc;
        entry.compressedSize = compSize;
        entry.uncompressedSize = uncompSize;
        entry.localHeaderOffset = localOffset;

        m_entries.insert(fileName, entry);
        m_fileList.append(fileName);

        cdPtr += 46 + fnLen + extraLen + commentLen;
    }

    return !m_entries.isEmpty();
}

QByteArray ZipReader::extractFile(const QString &fileName) const
{
    if (!m_entries.contains(fileName)) {
        return {};
    }

    const Entry entry = m_entries.value(fileName);
    if (entry.uncompressedSize == 0) {
        return {};
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    // Read Local File Header (LFH) at localHeaderOffset
    if (!file.seek(entry.localHeaderOffset)) {
        return {};
    }

    char lfh[30];
    if (file.read(lfh, 30) != 30) {
        return {};
    }

    if (std::memcmp(lfh, "PK\x03\x04", 4) != 0) {
        return {};
    }

    const uchar *lfhPtr = reinterpret_cast<const uchar*>(lfh);
    const uint16_t fnLen = qFromLittleEndian<uint16_t>(lfhPtr + 26);
    const uint16_t extraLen = qFromLittleEndian<uint16_t>(lfhPtr + 28);

    const qint64 dataOffset = static_cast<qint64>(entry.localHeaderOffset) + 30 + fnLen + extraLen;
    if (!file.seek(dataOffset)) {
        return {};
    }

    const QByteArray compressedData = file.read(entry.compressedSize);
    if (compressedData.size() != static_cast<int>(entry.compressedSize)) {
        return {};
    }

    if (entry.compressionMethod == 0) { // Stored (no compression)
        return compressedData;
    }

    if (entry.compressionMethod == 8) { // Deflate
        QByteArray decompressed;
        decompressed.resize(entry.uncompressedSize);

        z_stream strm{};
        strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressedData.constData()));
        strm.avail_in = static_cast<uInt>(compressedData.size());
        strm.next_out = reinterpret_cast<Bytef*>(decompressed.data());
        strm.avail_out = static_cast<uInt>(decompressed.size());

        // -MAX_WBITS instructs zlib to decompress a raw DEFLATE stream without zlib/gzip headers
        int ret = inflateInit2(&strm, -MAX_WBITS);
        if (ret != Z_OK) {
            return {};
        }

        ret = inflate(&strm, Z_FINISH);
        inflateEnd(&strm);

        if (ret != Z_STREAM_END && ret != Z_OK) {
            return {};
        }

        return decompressed;
    }

    return {};
}

