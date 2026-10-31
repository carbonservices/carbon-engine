#pragma once
#include <string>
#include <vector>
#include <memory>
#include <fpdfview.h>
#include <tesseract/baseapi.h>

struct DocumentChunk {
    int pageNumber;
    std::string text;
};

class PageTextExtractor {
public:
    virtual ~PageTextExtractor() = default;
    virtual bool canHandle(FPDF_TEXTPAGE textPage) const = 0;
    virtual std::string extract(FPDF_PAGE page, FPDF_TEXTPAGE textPage) = 0;
};

class NativeTextExtractor : public PageTextExtractor {
public:
    bool canHandle(FPDF_TEXTPAGE textPage) const override;
    std::string extract(FPDF_PAGE page, FPDF_TEXTPAGE textPage) override;
};

class OcrTextExtractor : public PageTextExtractor {
public:
    OcrTextExtractor();
    ~OcrTextExtractor() override;

    bool canHandle(FPDF_TEXTPAGE textPage) const override;
    std::string extract(FPDF_PAGE page, FPDF_TEXTPAGE textPage) override;
    std::string extractFromImage(const std::string& imagePath);

private:
    tesseract::TessBaseAPI _tess;
};

class DocumentProcessor {
public:
    DocumentProcessor();
    ~DocumentProcessor();

    std::vector<DocumentChunk> processPdf(const std::string& pdfPath);
    std::vector<DocumentChunk> processText(const std::string& txtPath);
    std::vector<DocumentChunk> processImage(const std::string& imgPath);
    std::vector<DocumentChunk> processExcel(const std::string& xlsxPath);
    std::vector<DocumentChunk> processDocument(const std::string& filePath);

    static std::vector<DocumentChunk> chunkText(int pageNumber, const std::string& text);

private:
    std::vector<std::unique_ptr<PageTextExtractor>> _extractors;
    std::unique_ptr<OcrTextExtractor> _imageOcr;
};
