<img src="https://avatars.githubusercontent.com/u/309081559" align="right" height="100" alt="Carbon Services Project"/>

# Carbon Engine

Native C++ engine for on-device ESG / BRSR data processing and report generation.
Supports following use-cases:
1. Document ingestion (supported extensions - pdf, jpg, png, txt, csv, xlsx)
2. BRSR report generation

## Architecture

- API: HTTP endpoints
- Queue: in-process polling job queue backed by SQLite; runs ingestion off the network thread
- Ingestion: PDF text extraction, Tesseract OCR for scanned / image-only pages
- Inference: Runs a generative model and a small embedding model using llama.cpp
- Vector search: FAISS in-process.
- Storage: SQLite

## Dependencies

| Dependency | Purpose |
|---|---|
| Qt 6.10 | Core, HttpServer, Sql, Concurrent |
| Visual Studio 2022 | MSVC toolchain (Windows) |
| llama.cpp | Local LLM inference |
| FAISS + Intel MKL | Vector search |
| PDFium | PDF text extraction |
| Tesseract + Leptonica | OCR |

## Building (Windows)

```
cmake -S . -B __Builds/x64/_build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.10.0/msvc2022_64"
cmake --build __Builds/x64/_build --config Release --target carbon-engine
```

**Binaries and libraries**:

- `__Builds/<arch>/bin`: executable and runtime DLLs
- `__Builds/<arch>/lib`: static libraries

## Running

```
CARBON_ENGINE_GEN_MODEL=models/qwen2.5-7b-instruct-q4_k_m.gguf \
CARBON_ENGINE_EMBD_MODEL=models/bge-small-en-v1.5-f16.gguf \
__Builds/x64/bin/carbon-engine.exe
```

Both env vars are optional and default to `models/qwen2.5-7b-instruct-q4_k_m.gguf` / `models/bge-small-en-v1.5-f16.gguf`. The server listens on `http://localhost:8080`.

### API

| API | Payload | Description |
| --- | --- | --- |
| `POST /api/ingest` | `{"file_path": "<path to PDF>"}` | Queues document; background worker extracts text / OCR, embeds each page, and indexes in database |
| `POST /api/chat` | `{"prompt": "<text>"}` | Embeds prompt; retrieves nearest matches from the vector index, and streams a completion from the generative model |
