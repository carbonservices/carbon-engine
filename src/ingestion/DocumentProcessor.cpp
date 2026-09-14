#include "DocumentProcessor.h"
#include <fpdf_text.h>
#include <iostream>
#include <QtGlobal>

constexpr int kNativeTextCharThreshold = 32;

bool NativeTextExtractor::canHandle(FPDF_TEXTPAGE textPage) const {
    return FPDFText_CountChars(textPage) > kNativeTextCharThreshold;
}

std::string NativeTextExtractor::extract(FPDF_PAGE /*page*/, FPDF_TEXTPAGE textPage) {
    int charCount = FPDFText_CountChars(textPage);
    std::vector<unsigned short> buffer(charCount + 1);
    FPDFText_GetText(textPage, 0, charCount, buffer.data());

    std::string text;
    text.reserve(charCount);
    for (int c = 0; c < charCount; ++c) {
        text += static_cast<char>(buffer[c]);
    }
    return text;
}

OcrTextExtractor::OcrTextExtractor() {
    _tess.Init(Q_NULLPTR, "eng", tesseract::OEM_LSTM_ONLY);
    _tess.SetPageSegMode(tesseract::PSM_AUTO);
}

OcrTextExtractor::~OcrTextExtractor() {
    _tess.End();
}

bool OcrTextExtractor::canHandle(FPDF_TEXTPAGE textPage) const {
    return FPDFText_CountChars(textPage) <= kNativeTextCharThreshold;
}

std::string OcrTextExtractor::extract(FPDF_PAGE page, FPDF_TEXTPAGE /*textPage*/) {
    int width = static_cast<int>(FPDF_GetPageWidth(page));
    int height = static_cast<int>(FPDF_GetPageHeight(page));

    FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, 0);
    FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF);
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, 0);

    const void* buffer = FPDFBitmap_GetBuffer(bitmap);
    int stride = FPDFBitmap_GetStride(bitmap);

    _tess.SetImage(static_cast<const unsigned char*>(buffer), width, height, 4, stride);
    char* ocrText = _tess.GetUTF8Text();
    std::string result(ocrText ? ocrText : "");

    delete[] ocrText;
    FPDFBitmap_Destroy(bitmap);
    return result;
}

DocumentProcessor::DocumentProcessor() {
    FPDF_InitLibrary();
    _extractors.push_back(std::make_unique<NativeTextExtractor>());
    _extractors.push_back(std::make_unique<OcrTextExtractor>());
}

DocumentProcessor::~DocumentProcessor() {
    FPDF_DestroyLibrary();
}

std::vector<DocumentChunk> DocumentProcessor::processPdf(const std::string& pdfPath) {
    std::vector<DocumentChunk> chunks;
    FPDF_DOCUMENT doc = FPDF_LoadDocument(pdfPath.c_str(), Q_NULLPTR);
    if (!doc) return chunks;

    int pageCount = FPDF_GetPageCount(doc);
    for (int i = 0; i < pageCount; ++i) {
        FPDF_PAGE page = FPDF_LoadPage(doc, i);
        FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);

        std::string extractedText;
        for (auto& extractor : _extractors) {
            if (extractor->canHandle(textPage)) {
                extractedText = extractor->extract(page, textPage);
                break;
            }
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
