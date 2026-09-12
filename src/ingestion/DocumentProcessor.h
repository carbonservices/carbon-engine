#pragma once
#include <string>
#include <vector>
#include <fpdfview.h>
#include <tesseract/baseapi.h>

struct DocumentChunk {
    int pageNumber;
    std::string text;
};

class DocumentProcessor {
public:
    DocumentProcessor();
    ~DocumentProcessor();

    std::vector<DocumentChunk> processPdf(const std::string& pdfPath);

private:
    tesseract::TessBaseAPI m_tess;
    std::string performOcrOnPage(FPDF_PAGE page, int width, int height);
};