<img src="https://avatars.githubusercontent.com/u/309081559" align="right" height="50" alt="Carbon Services Project"/>

# Carbon Engine

Native C++ engine for on-device ESG / BRSR data processing and report generation.
Supports following use-cases:
1. Document ingestion (supported extensions - pdf, jpg, png, txt, csv, xlsx)
2. BRSR report generation

## Architecture

- `src/libs` holds the reusable engine code (built as the static library `carbon-engine-libs`)
- `src/apps` holds the two executables that link against it

- API: HTTP endpoints (`src/libs/api`)
- Queue: in-process polling job queue backed by SQLite; runs ingestion off the network thread (`src/libs/queue`)
- Ingestion: PDF text extraction, Tesseract OCR for scanned / image-only pages (`src/libs/ingestion`)
- Inference: Runs a generative model and a small embedding model using llama.cpp (`src/libs/inference`)
- Vector search: FAISS in-process (`src/libs/vector`)
- Storage: SQLite (`src/libs/db`)
- Logging: per-module, date-timestamped log files (`src/libs/logging`), see [Logging](#logging)
- Apps: `src/apps/server` (the HTTP API executable, `carbon-engine`) and `src/apps/qml-client` (a Qt Quick desktop client, `carbon-engine-client`)

## Dependencies

| Dependency | Purpose |
|---|---|
| Qt 6.10 | Core, HttpServer, Sql, Concurrent, Quick, QuickControls2, Network |
| Visual Studio 2022 | MSVC toolchain (Windows) |
| llama.cpp | Local LLM inference |
| FAISS + Intel MKL | Vector search |
| PDFium | PDF text extraction |
| Tesseract + Leptonica | OCR |
| NASM | Assembler required by libjpeg-turbo's SIMD build; auto-installed via `winget` if not found on `PATH` or in the well-known NASM install directories |

Third-party dependencies are fetched via CMake `FetchContent`. Pinned dependencies (FAISS, PDFium, Tesseract, Leptonica, zlib, libjpeg-turbo, libpng) are only downloaded once. CMake reuses the existing source directory under `third-party/<name>-src` on subsequent configures. The one exception is llama.cpp, which is pinned to `GIT_TAG master` (a moving branch, not a fixed tag) and so is checked for updates on every configure by design.

## Building (Windows)

```
cmake -S . -B __Builds/x64/_build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.10.0/msvc2022_64"
cmake --build __Builds/x64/_build --config Release --target carbon-engine --target carbon-engine-client
```

**Binaries and libraries**:

- `__Builds/<arch>/bin`: executables and runtime DLLs (`carbon-engine.exe`, `carbon-engine-client.exe`)
- `__Builds/<arch>/lib`: static libraries

## Logging

Every module logs through `src/libs/logging` (`Logger` façade over a `FileLogSink`). Each module writes to its own date-timestamped file: `log/<ModuleName>_<yyyy-MM-dd>.log`. The `log` directory is created next to the running executable when writable (e.g. the project root in a dev/build tree); otherwise it falls back to `%TEMP%/carbon-engine/log`.

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
| `POST /api/ingest` | `{"file_path": "<path to PDF>"}` | Queues document; returns `{"job_id": <id>}`. Background worker extracts text / OCR, embeds each page, and indexes in database |
| `GET /api/jobs?job_id=<id>` | None | Returns `{"status": "PENDING"\|"PROCESSING"\|"COMPLETED"\|"FAILED"}` for the given job |
| `POST /api/report` | `{"job_id": <id>}` | Builds a BRSR-style report from the job's extracted chunks and returns `{"report": "<text>"}` |
| `POST /api/chat` | `{"prompt": "<text>"}` | Embeds prompt; retrieves nearest matches from the vector index, and streams a completion from the generative model |

## QML client

`carbon-engine-client` (`src/apps/qml-client`) is a Qt Quick desktop app that talks to the `carbon-engine` server over HTTP: pick a document, upload it, watch the ingest pipeline stages update live, then view the generated report once the job completes.

```
__Builds/x64/bin/carbon-engine-client.exe
```

Requires `carbon-engine.exe` to be running on `http://localhost:8080`.
