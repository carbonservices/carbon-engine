// src/ingestion/DocumentProcessor.cpp
#include "DocumentProcessor.h"
#include <fpdf_text.h>
#include <iostream>

DocumentProcessor::DocumentProcessor() {
    FPDF_InitLibrary();
    m_tess.Init(NULL, "eng", tesseract::OEM_LSTM_ONLY);
    m_tess.SetPageSegMode(tesseract::PSM_AUTO);
}

DocumentProcessor::~DocumentProcessor() {
    m_tess.End();
    FPDF_DestroyLibrary();
}

std::vector<DocumentChunk> DocumentProcessor::processPdf(const std::string& pdfPath) {
    std::vector<DocumentChunk> chunks;
    FPDF_DOCUMENT doc = FPDF_LoadDocument(pdfPath.c_str(), NULL);
    if (!doc) return chunks;

    int pageCount = FPDF_GetPageCount(doc);
    for (int i = 0; i < pageCount; ++i) {
        FPDF_PAGE page = FPDF_LoadPage(doc, i);
        FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);

        int charCount = FPDFText_CountChars(textPage);
        std::string extractedText;

        if (charCount > 32) { // Native text layer exists
            std::vector<unsigned short> buffer(charCount + 1);
            FPDFText_GetText(textPage, 0, charCount, buffer.data());
            for (int c = 0; c < charCount; ++c) {
                extractedText += static_cast<char>(buffer[c]);
            }
        } else { // Fallback to Tesseract OCR
            int width = static_cast<int>(FPDF_GetPageWidth(page));
            int height = static_cast<int>(FPDF_GetPageHeight(page));
            extractedText = performOcrOnPage(page, width, height);
        }

        if (!extractedText.empty()) {
            chunks.push_back({i + 1, extractedText});
        }

        FPDFText_ClosePage(textPage);
        FPDF_ClosePage(page);
    }

    FPDF_CloseDocument(doc);
    return chunks;
}

std::string DocumentProcessor::performOcrOnPage(FPDF_PAGE page, int width, int height) {
    FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, 0);
    FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF);
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, 0);

    const void* buffer = FPDFBitmap_GetBuffer(bitmap);
    int stride = FPDFBitmap_GetStride(bitmap);

    m_tess.SetImage(static_cast<const unsigned char*>(buffer), width, height, 4, stride);
    char* ocrText = m_tess.GetUTF8Text();
    std::string result(ocrText ? ocrText : "");

    delete[] ocrText;
    FPDFBitmap_Destroy(bitmap);
    return result;
}