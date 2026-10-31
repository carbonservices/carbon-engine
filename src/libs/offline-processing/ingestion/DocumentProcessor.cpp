#include "DocumentProcessor.h"
#include "logging/Logger.h"
#include <fpdf_text.h>
#include <fpdf_edit.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QXmlStreamReader>
#include "ingestion/ZipReader.h"
#include <QString>
#include <QtGlobal>
#include <QAtomicInt>
#include <QMutex>
#include <QMutexLocker>
#include <algorithm>
#include <cctype>
#include <stdexcept>

constexpr int kNativeTextCharThreshold = 32;

bool NativeTextExtractor::canHandle(FPDF_TEXTPAGE textPage) const
{
    return FPDFText_CountChars(textPage) > kNativeTextCharThreshold;
}

std::string NativeTextExtractor::extract(FPDF_PAGE /*page*/, FPDF_TEXTPAGE textPage)
{
    int charCount = FPDFText_CountChars(textPage);
    std::vector<unsigned short> buffer(charCount + 1);
    FPDFText_GetText(textPage, 0, charCount, buffer.data());

    // FPDFText_GetText fills UTF-16LE code units; QString::fromUtf16 handles
    // the transcode to UTF-8 correctly instead of truncating each unit to a
    // single byte (which mangles any non-ASCII text).
    QString text = QString::fromUtf16(reinterpret_cast<const char16_t *>(buffer.data()), charCount);
    return text.toStdString();
}

static QString resolveTessdataPath()
{
    static Logger s_log("DocumentProcessor");
    const QString appDir = QCoreApplication::applicationDirPath();

    // ─────────────────────────────────────────────────────────────
    // 1. Source Build Mode: Cloned project folder directory structure
    // ─────────────────────────────────────────────────────────────
#ifdef CARBON_PROJECT_SOURCE_DIR
    const QString compileTimeSource = QDir::cleanPath(QStringLiteral(CARBON_PROJECT_SOURCE_DIR));
    if (QFile::exists(compileTimeSource + "/CMakeLists.txt") &&
        QFile::exists(compileTimeSource + "/tessdata/eng.traineddata")) {
        const QString path = compileTimeSource + "/tessdata";
        s_log.info(QString("Tessdata resolved from source repository: %1").arg(path));
        return path;
    }
#endif

    // Upward directory traversal to detect cloned repository root
    QDir searchDir(appDir);
    for (int depth = 0; depth < 5; ++depth) {
        if (searchDir.exists("CMakeLists.txt") && searchDir.exists("src")) {
            const QString candidate = searchDir.filePath("tessdata");
            if (QFile::exists(candidate + "/eng.traineddata")) {
                s_log.info(QString("Tessdata resolved from cloned project root: %1").arg(candidate));
                return candidate;
            }
        }
        if (!searchDir.cdUp()) {
            break;
        }
    }

    // Build directory output copy (<build_dir>/bin/tessdata)
    if (QFile::exists(appDir + "/tessdata/eng.traineddata")) {
        s_log.info(QString("Tessdata resolved from build directory: %1/tessdata").arg(appDir));
        return appDir + "/tessdata";
    }

    // ─────────────────────────────────────────────────────────────
    // 2. Installed Application Mode: Deterministic install locations
    // ─────────────────────────────────────────────────────────────
#ifdef _WIN32
    // Windows NSIS Install Layout:
    //   $INSTDIR\bin\carbon-engine.exe
    //   $INSTDIR\bin\tessdata\eng.traineddata
    if (QFile::exists(appDir + "/tessdata/eng.traineddata")) {
        s_log.info(QString("Tessdata resolved from Windows install path: %1/tessdata").arg(appDir));
        return appDir + "/tessdata";
    }
    // Secondary install layout ($INSTDIR\share\tessdata)
    if (QFile::exists(appDir + "/../share/tessdata/eng.traineddata")) {
        QString path = QDir::cleanPath(appDir + "/../share/tessdata");
        s_log.info(QString("Tessdata resolved from Windows share path: %1").arg(path));
        return path;
    }
    // Root install layout ($INSTDIR\tessdata)
    if (QFile::exists(appDir + "/../tessdata/eng.traineddata")) {
        QString path = QDir::cleanPath(appDir + "/../tessdata");
        s_log.info(QString("Tessdata resolved from Windows root path: %1").arg(path));
        return path;
    }
#else
    // Linux Debian Install Layout:
    //   /usr/bin/carbon-engine -> /usr/share/tessdata/eng.traineddata
    //   /opt/carbon-esg/bin/carbon-engine -> /opt/carbon-esg/share/tessdata/eng.traineddata
    if (QFile::exists(appDir + "/../share/tessdata/eng.traineddata")) {
        QString path = QDir::cleanPath(appDir + "/../share/tessdata");
        s_log.info(QString("Tessdata resolved from Linux install share: %1").arg(path));
        return path;
    }
    if (QFile::exists("/usr/share/tessdata/eng.traineddata")) {
        s_log.info("Tessdata resolved from system /usr/share/tessdata");
        return "/usr/share/tessdata";
    }
    if (QFile::exists("/usr/share/tesseract-ocr/5/tessdata/eng.traineddata")) {
        s_log.info("Tessdata resolved from distro /usr/share/tesseract-ocr/5/tessdata");
        return "/usr/share/tesseract-ocr/5/tessdata";
    }
    if (QFile::exists("/usr/share/tesseract-ocr/4.00/tessdata/eng.traineddata")) {
        s_log.info("Tessdata resolved from distro /usr/share/tesseract-ocr/4.00/tessdata");
        return "/usr/share/tesseract-ocr/4.00/tessdata";
    }
    if (QFile::exists(appDir + "/tessdata/eng.traineddata")) {
        s_log.info(QString("Tessdata resolved from application directory: %1/tessdata").arg(appDir));
        return appDir + "/tessdata";
    }
#endif

    s_log.error("Fatal: Unable to resolve tessdata directory in source or install locations!");
    return QString();
}

OcrTextExtractor::OcrTextExtractor()
{
    QString tessPath = resolveTessdataPath();
    QByteArray tessPathBytes = tessPath.toUtf8();
    const char* tessDataPrefix = tessPathBytes.isEmpty() ? Q_NULLPTR : tessPathBytes.constData();

    _tess.Init(tessDataPrefix, "eng", tesseract::OEM_LSTM_ONLY);
    _tess.SetPageSegMode(tesseract::PSM_AUTO);
}

OcrTextExtractor::~OcrTextExtractor()
{
    _tess.End();
}

bool OcrTextExtractor::canHandle(FPDF_TEXTPAGE textPage) const
{
    return FPDFText_CountChars(textPage) <= kNativeTextCharThreshold;
}

std::string OcrTextExtractor::extract(FPDF_PAGE page, FPDF_TEXTPAGE /*textPage*/)
{
    if (!page) return "";

    // 1. Fast PDF Object Check: If page contains 0 objects, it is mathematically blank
    if (FPDFPage_CountObjects(page) == 0) {
        return "";
    }

    constexpr float kOcrScale = 2.0f; // Render at ~144-150 DPI for accurate Tesseract recognition
    int width = static_cast<int>(FPDF_GetPageWidth(page) * kOcrScale);
    int height = static_cast<int>(FPDF_GetPageHeight(page) * kOcrScale);
    if (width <= 0 || height <= 0) return "";

    FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, 0);
    if (!bitmap) return "";

    FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF);
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, 0);

    const void *buffer = FPDFBitmap_GetBuffer(bitmap);
    int stride = FPDFBitmap_GetStride(bitmap);

    // 2. Fast Ink Check: If >= 99.85% of sampled pixels are white, skip OCR
    const uint32_t *pixels = reinterpret_cast<const uint32_t *>(buffer);
    int totalPixels = width * height;
    int step = std::max(1, totalPixels / 1000);
    int nonWhite = 0;
    int samples = 0;
    for (int p = 0; p < totalPixels; p += step) {
        samples++;
        if ((pixels[p] & 0x00FFFFFF) != 0x00FFFFFF) {
            nonWhite++;
        }
    }
    if (samples > 0 && (static_cast<float>(nonWhite) / samples) < 0.0015f) {
        FPDFBitmap_Destroy(bitmap);
        return "";
    }

    _tess.SetImage(static_cast<const unsigned char *>(buffer), width, height, 4, stride);
    char *ocrText = _tess.GetUTF8Text();
    std::string result(ocrText ? ocrText : "");

    delete[] ocrText;
    FPDFBitmap_Destroy(bitmap);
    return result;
}

std::string OcrTextExtractor::extractFromImage(const std::string& imagePath)
{
    QImage img(QString::fromStdString(imagePath));
    if (img.isNull()) {
        return "";
    }

    QImage argb = img.convertToFormat(QImage::Format_RGBA8888);
    _tess.SetImage(reinterpret_cast<const unsigned char*>(argb.bits()),
                   argb.width(), argb.height(), 4, static_cast<int>(argb.bytesPerLine()));
    char* ocrText = _tess.GetUTF8Text();
    std::string result(ocrText ? ocrText : "");
    delete[] ocrText;
    return result;
}

static QAtomicInt s_pdfiumRefCount = 0;
static QMutex s_pdfiumMutex;

DocumentProcessor::DocumentProcessor()
{
    {
        QMutexLocker locker(&s_pdfiumMutex);
        if (s_pdfiumRefCount.fetchAndAddRelaxed(1) == 0) {
            FPDF_InitLibrary();
        }
    }
    _extractors.push_back(std::make_unique<NativeTextExtractor>());
    _extractors.push_back(std::make_unique<OcrTextExtractor>());
}

DocumentProcessor::~DocumentProcessor()
{
    QMutexLocker locker(&s_pdfiumMutex);
    if (!s_pdfiumRefCount.deref()) {
        FPDF_DestroyLibrary();
    }
}

std::vector<DocumentChunk> DocumentProcessor::chunkText(int pageNumber, const std::string& extractedText)
{
    std::vector<DocumentChunk> chunks;
    if (extractedText.empty()) {
        return chunks;
    }

    constexpr size_t maxChars = 1500;
    constexpr size_t overlap = 300;
    size_t start = 0;

    while (start < extractedText.length())
    {
        size_t end = start + maxChars;
        if (end < extractedText.length())
        {
            size_t lookbackMin = (end > 200 + start) ? (end - 200) : start;
            size_t lastNewline = extractedText.rfind('\n', end);
            if (lastNewline != std::string::npos && lastNewline > lookbackMin)
            {
                end = lastNewline;
            }
            else
            {
                size_t lastWhitespace = extractedText.find_last_of(" \n\r\t", end);
                if (lastWhitespace != std::string::npos && lastWhitespace > start)
                {
                    end = lastWhitespace;
                }
            }
        }
        else
        {
            end = extractedText.length();
        }

        std::string rawChunk = extractedText.substr(start, end - start);
        size_t firstNonWs = rawChunk.find_first_not_of(" \t\n\r");
        if (firstNonWs != std::string::npos)
        {
            size_t lastNonWs = rawChunk.find_last_not_of(" \t\n\r");
            std::string cleanChunk = rawChunk.substr(firstNonWs, lastNonWs - firstNonWs + 1);
            if (cleanChunk.length() >= 20)
            {
                chunks.push_back({pageNumber, std::move(cleanChunk)});
            }
        }

        if (end >= extractedText.length())
        {
            break;
        }

        size_t nextStart = (end > overlap + start) ? (end - overlap) : end;
        while (nextStart < end && !std::isspace(static_cast<unsigned char>(extractedText[nextStart])))
        {
            nextStart++;
        }
        while (nextStart < extractedText.length() && std::isspace(static_cast<unsigned char>(extractedText[nextStart])))
        {
            nextStart++;
        }
        if (nextStart <= start)
        {
            nextStart = end;
        }
        start = nextStart;
    }
    return chunks;
}

std::vector<DocumentChunk> DocumentProcessor::processPdf(const std::string &pdfPath)
{
    std::vector<DocumentChunk> chunks;
    FPDF_DOCUMENT doc = FPDF_LoadDocument(pdfPath.c_str(), Q_NULLPTR);
    if (!doc)
        throw std::runtime_error("Failed to load PDF: " + pdfPath);

    int pageCount = FPDF_GetPageCount(doc);
    for (int i = 0; i < pageCount; ++i)
    {
        FPDF_PAGE page = FPDF_LoadPage(doc, i);
        if (!page)
            continue;

        FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
        if (!textPage)
        {
            FPDF_ClosePage(page);
            continue;
        }

        std::string extractedText;
        for (auto &extractor : _extractors)
        {
            if (extractor->canHandle(textPage))
            {
                extractedText = extractor->extract(page, textPage);
                break;
            }
        }

        if (!extractedText.empty())
        {
            auto pageChunks = chunkText(i + 1, extractedText);
            chunks.insert(chunks.end(), pageChunks.begin(), pageChunks.end());
        }

        FPDFText_ClosePage(textPage);
        FPDF_ClosePage(page);
    }

    FPDF_CloseDocument(doc);
    return chunks;
}

std::vector<DocumentChunk> DocumentProcessor::processText(const std::string &txtPath)
{
    QFile file(QString::fromStdString(txtPath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    QString text = QString::fromUtf8(file.readAll());
    return chunkText(1, text.toStdString());
}

std::vector<DocumentChunk> DocumentProcessor::processImage(const std::string &imgPath)
{
    if (!_imageOcr) {
        _imageOcr = std::make_unique<OcrTextExtractor>();
    }
    std::string text = _imageOcr->extractFromImage(imgPath);
    if (text.empty()) {
        return {};
    }
    return chunkText(1, text);
}

std::vector<DocumentChunk> DocumentProcessor::processExcel(const std::string &xlsxPath)
{
    QString qPath = QString::fromStdString(xlsxPath);
    ZipReader zip(qPath);
    if (!zip.isOpen()) {
        return {};
    }

    // 1. Read shared strings
    QVector<QString> sharedStrings;
    QByteArray ssData = zip.extractFile("xl/sharedStrings.xml");
    if (!ssData.isEmpty()) {
        QXmlStreamReader xml(ssData);
        QString currentString;
        bool inText = false;
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement()) {
                if (xml.name() == QLatin1String("t")) {
                    inText = true;
                }
            } else if (xml.isCharacters() && inText) {
                currentString += xml.text().toString();
            } else if (xml.isEndElement()) {
                if (xml.name() == QLatin1String("t")) {
                    inText = false;
                } else if (xml.name() == QLatin1String("si")) {
                    sharedStrings.append(currentString.trimmed());
                    currentString.clear();
                }
            }
        }
    }

    // 2. Iterate worksheets
    std::vector<DocumentChunk> allChunks;
    int sheetNum = 1;

    for (const auto &filePath : zip.fileList()) {
        if (filePath.startsWith("xl/worksheets/sheet") && filePath.endsWith(".xml")) {
            QByteArray sheetData = zip.extractFile(filePath);
            if (sheetData.isEmpty()) continue;

            QXmlStreamReader xml(sheetData);
            QString sheetText;
            QString currentRow;
            QString currentCellType;
            QString currentCellValue;
            bool inValue = false;
            bool inInlineText = false;

            while (!xml.atEnd()) {
                xml.readNext();
                if (xml.isStartElement()) {
                    if (xml.name() == QLatin1String("row")) {
                        currentRow.clear();
                    } else if (xml.name() == QLatin1String("c")) {
                        currentCellType = xml.attributes().value("t").toString();
                        currentCellValue.clear();
                    } else if (xml.name() == QLatin1String("v")) {
                        inValue = true;
                    } else if (xml.name() == QLatin1String("t")) {
                        inInlineText = true;
                    }
                } else if (xml.isCharacters()) {
                    if (inValue || inInlineText) {
                        currentCellValue += xml.text().toString();
                    }
                } else if (xml.isEndElement()) {
                    if (xml.name() == QLatin1String("v")) {
                        inValue = false;
                    } else if (xml.name() == QLatin1String("t")) {
                        inInlineText = false;
                    } else if (xml.name() == QLatin1String("c")) {
                        QString cellText = currentCellValue.trimmed();
                        if (currentCellType == "s") {
                            int idx = cellText.toInt();
                            if (idx >= 0 && idx < sharedStrings.size()) {
                                cellText = sharedStrings[idx];
                            }
                        }
                        if (!cellText.isEmpty()) {
                            if (!currentRow.isEmpty()) {
                                currentRow += " | ";
                            }
                            currentRow += cellText;
                        }
                    } else if (xml.name() == QLatin1String("row")) {
                        if (!currentRow.trimmed().isEmpty()) {
                            sheetText += currentRow.trimmed() + "\n";
                        }
                    }
                }
            }

            if (!sheetText.isEmpty()) {
                auto chunks = chunkText(sheetNum, sheetText.toStdString());
                allChunks.insert(allChunks.end(), chunks.begin(), chunks.end());
            }
            sheetNum++;
        }
    }
    return allChunks;
}

std::vector<DocumentChunk> DocumentProcessor::processDocument(const std::string &filePath)
{
    QFileInfo fi(QString::fromStdString(filePath));
    QString ext = fi.suffix().toLower();

    if (ext == "pdf") {
        return processPdf(filePath);
    } else if (ext == "xlsx") {
        return processExcel(filePath);
    } else if (ext == "txt" || ext == "csv" || ext == "tsv" || ext == "md" || ext == "json") {
        return processText(filePath);
    } else if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "tiff") {
        return processImage(filePath);
    }
    return processPdf(filePath);
}
