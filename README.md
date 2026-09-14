# Carbon Engine

A single native C++ binary for on-device ESG/BRSR data processing: document ingestion (PDF/OCR), local LLM inference, vector search, and a small HTTP API.

## Architecture

One process, everything embedded:

- **API** (`src/api`) — `QHttpServer`-based HTTP endpoints (`/api/ingest`, `/api/chat`).
- **Queue** (`src/queue`) — in-process polling job queue backed by SQLite; runs ingestion off the network thread via `QtConcurrent`.
- **Ingestion** (`src/ingestion`) — PDF text extraction via PDFium, falling back to Tesseract OCR for scanned/image-only pages.
- **Inference** (`src/inference`) — `llama.cpp` embedded directly for both a generative model and a small embedding model (quantized GGUF).
- **Vector search** (`src/vector`) — FAISS `IndexHNSW` in-process.
- **Storage** (`src/db`) — SQLite via `QtSql`, migrated in-code (`ingestion_jobs`, `document_chunks`).

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

Final artifacts land in clean sibling directories, ready for installer packaging:

- `__Builds/<arch>/bin` — executable and runtime DLLs
- `__Builds/<arch>/lib` — import libraries

Only `bin/` and `lib/` should be picked up by an installer/packaging step — never `_build/`.

## Running

```
CARBON_ENGINE_GEN_MODEL=models/qwen2.5-7b-instruct-q4_k_m.gguf \
CARBON_ENGINE_EMBD_MODEL=models/bge-small-en-v1.5-f16.gguf \
__Builds/x64/bin/carbon-engine.exe
```

Both env vars are optional and default to `models/qwen2.5-7b-instruct-q4_k_m.gguf` / `models/bge-small-en-v1.5-f16.gguf` relative to the working directory. The server listens on `http://localhost:8080`.

### API

- `POST /api/ingest` — `{"file_path": "<path to PDF>"}` → queues the document; a background worker extracts text/OCR, embeds each page, and indexes it.
- `POST /api/chat` — `{"prompt": "<text>"}` → embeds the prompt, retrieves nearest matches from the vector index, and streams a completion from the generative model (returned as a single JSON response with `response` and `context_matches`).
