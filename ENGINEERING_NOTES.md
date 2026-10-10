# Engineering Notes — Carbon Engine

Consolidated findings from a review of the report-generation pipeline (`src/libs`) and the
QML desktop client (`src/apps/qml-client`). This file is the design/decision record; day-to-day
checkable status lives in `IMPLEMENTATION_PLAN.md` — see that file for what's done vs. pending.

**Status note (2026-10-10):** Several items below have been implemented since this file was first
written: the `template.html` path/CMake-copy fix, database indices + FK enforcement, the
`src/libs/system` telemetry module, the `GET /api/system` endpoint, `SystemStatusPoller` wiring,
model load/unload in `LlamaEngine`, and `hardware_concurrency()`-based thread count. Those sections
have been removed from this file; see `IMPLEMENTATION_PLAN.md` Phase 1 (complete) and Phase 0
(partially complete) for details. Everything retained below is either still-open architecture
(report generation redesign, per-job retrieval, ingestion staging) or still-outstanding hygiene
(CMake pinning, source-tree build artifacts).

## Critical finding — report generation is architecturally broken, not just inefficient

**The generation step cannot produce correct output for the template it renders, structurally,
regardless of model quality or tuning.** This is not a bug to patch; it's a scale mismatch between
what the template needs and what the code asks the LLM for.

### Finding A — the template needs ~300 fields and 73 loops; the prompt asks for far fewer

`template.html` (`src/apps/qml-client/template.html`) contains **301** `{{ variable }}` placeholders
and **73** `{% for %}` loop blocks (business activities, employee tables, CSR projects, principle-wise
ESG indicators P1–P9, GHG/water/waste/energy tables, chart data tables, etc. — a full BRSR report
structure spanning Sections A, B, and C with 9 "principles").

`handleReport`'s extraction prompt has grown since this was first flagged (now merged into a larger
`kDefaultBrsrSchemaJson`, `HTTPServer.cpp:124`+), but it is still generated in a **single**
`streamCompletion` call against one hardcoded RAG query. Inja does not error on missing keys by
default — undefined variables render as empty/`null` and `{% for %}` over an undefined array
renders zero rows. So the *symptom* is not a crash: it's a mostly-empty, 60-70-page BRSR report
shell with a handful of fields populated — indistinguishable at a glance from "the model didn't
extract anything."

**This is not fixable by prompt-tuning, a bigger context window, or a better model.** Getting an
LLM to reliably emit ~300 correctly-keyed fields and 73 correctly-shaped arrays in one shot, from a
single RAG context assembled from one hardcoded query, is not a realistic ask of a 7B model (or
most larger ones) in one pass.

### Finding B — RAG search has no per-job/per-document scoping (cross-document contamination)

`VectorIndex::search` and its caller in `handleReport` perform a **global** top-k search over
*every* chunk ever ingested from *every* document, across all jobs. `jobId` is extracted from the
request but is never passed to `_index.search(...)`, and `Database::getChunkByVectorId` looks up
chunks by `vector_id` alone with no `job_id` predicate either.

Concretely: if a user ingests "TATA Steel BRSR 2026" and later "JSW Steel Sustainability 2025",
requesting a report for the TATA job can retrieve and inject JSW's chunks into the TATA report's
context — and vice versa — because the single FAISS index has no concept of "which document does
this vector belong to" at query time.

This is a **correctness bug, not a performance one** — it will silently blend data from unrelated
companies/documents into a single generated report, which for a regulatory ESG disclosure product
is a serious data-integrity problem.

**Still not fixed as of this review** — see `IMPLEMENTATION_PLAN.md` Phase 3.

### Finding C — the RAG query is a single hardcoded string, unrelated to the template's actual structure

`HTTPServer.cpp` hardcodes one query string for every report, regardless of which of the 9 BRSR
principles / which template section is being populated. Even a "redesign to multiple extraction
calls" fix needs more than just looping the same call 9 times — each principle needs its *own*
targeted RAG query (e.g. "water withdrawal and discharge data" for Principle 6, "employee wellbeing
and health insurance coverage" for Principle 3) to retrieve relevant chunks for that section at
all. This is tightly coupled to Finding A's resolution.

## Clarifying questions and decisions (D1–D6)

These were resolved through a Q&A round and now drive `IMPLEMENTATION_PLAN.md` Phases 2–4 directly.
Kept here as the rationale record; the actionable checklist lives in the plan doc.

**D1 — extraction fills what it can find; nothing is forced; a review UI sits between extraction
and HTML export.** Report generation becomes three stages:
1. **Extract** — per-section/field-group extraction calls populate a structured JSON object with
   per-field provenance, leaving fields `null`/absent where no supporting chunk was found.
2. **Review** — a new UI surface presents extracted fields alongside source chunk/graph context,
   and runs algorithmic validation checks (required-field presence, numeric sanity, cross-field
   consistency) before export, flagging failures as warnings the user must acknowledge.
3. **Export** — only at this point does the (human-reviewed/edited) JSON get merged into
   `template.html` via Inja.

`HttpServer::handleReport` needs to split into an extraction endpoint (structured JSON + warnings)
and a separate export endpoint (renders final HTML from possibly-edited JSON).

**D2 — retrieval and jobs are strictly per-document/per-job; no cross-job retrieval; ingestion
becomes a two-step "stage documents, then confirm" flow.**
- *Retrieval scoping*: per-job-partitioned `VectorIndex` (one instance per job, loaded on demand,
  evicted on idle/switch-away) rather than a filtered global index.
- *Upload/ingestion flow*: files picked via the add-document dialog should stage into
  `documentQueueModel` without calling the upload API immediately; a new explicit "Start"/"Confirm"
  action submits the whole staged batch as **one job containing multiple source files**. Requires
  `ingestion_jobs.file_path` (singular) to become a one-to-many `job_documents(job_id, file_path)`
  table.

**D3 — per-call targeted extraction, with a location-biased second pass and chunk-level
classification labels, rather than one all-at-once call or N independent from-scratch searches.**
- Each extraction call targets one section/field-group with its own RAG query.
- While processing one extraction call, record **candidate locations** — chunks that look relevant
  to a *different* field group than the one currently being queried — so the next field group's
  query consults that cheap pre-filter first instead of a cold full-corpus search.
- **Classification labels on chunks**: tag each chunk at ingestion time with topic labels (`water`,
  `emissions`, `csr`, `employee_wellbeing`, `governance`, etc.) via a lightweight heuristic
  classifier or a short LLM call; use label match to narrow the candidate set before falling back
  to vector similarity.
- Flagged as its own focused design pass once the structural Phase 2–4 changes are in place — not
  yet fully specified as an implementation.

**D4 — jobs are frozen once started; no re-ingestion/edit of source documents; updates require a
brand-new job.** Simplifies the data model: no "does re-ingestion clobber manual edits" logic
needed. A job's vector partition is write-once and never needs in-place updates, only
eviction/rebuild-on-demand from the persisted `embedding` BLOB column. Once a job is
confirmed/started, its document list should become read-only in the UI.

**D5 — keep the `GPU0` field; show `N/A` on machines without a GPU; real GPU metrics later.**
Implemented — see `IMPLEMENTATION_PLAN.md` Phase 1.

**D6 — server-side system lib inside `carbon-engine.exe`; both process-level and machine-wide
stats surfaced on the UI.** Implemented — see `IMPLEMENTATION_PLAN.md` Phase 1.

## Pipeline flow diagram

Two executables: `carbon-engine` (server, owns the DB/LLM/FAISS/HTTP) and
`carbon-engine-client` (Qt Quick GUI, talks to the server over `localhost:8080`).

```
┌─────────────────────┐        POST /api/ingest        ┌──────────────────────────────┐
│ carbon-engine-client │ ──────────────────────────────▶│ HttpServer::handleIngest     │
│  UploadClient        │                                 │  (QtConcurrent::run)         │
└─────────────────────┘                                 └──────────────┬───────────────┘
                                                                        │ Database::enqueueJob
                                                                        ▼
                                                          ┌──────────────────────────────┐
                                                          │ ingestion_jobs row: PENDING  │
                                                          └──────────────┬───────────────┘
                                                                         │ QTimer (1500ms)
                                                                         ▼
                                                          ┌──────────────────────────────┐
                                                          │ JobQueue::pollNextJob         │
                                                          │  (serialized via              │
                                                          │   atomic _isProcessing)       │
                                                          └──────────────┬───────────────┘
                                                                         │ QThreadPool::globalInstance()
                                                                         ▼
                               ┌─────────────────────────────────────────────────────────┐
                               │ DocumentProcessor::processPdf                            │
                               │  PDFium text extraction, OCR fallback (Tesseract)        │
                               │  → chunk (1500 chars, no overlap)                        │
                               └──────────────────────────┬────────────────────────────────┘
                                                           │ per chunk
                                                           ▼
                               ┌─────────────────────────────────────────────────────────┐
                               │ LlamaEngine::generateEmbedding (bge-small, _embdMutex)   │
                               └──────────────────────────┬────────────────────────────────┘
                                                           │
                                      ┌────────────────────┴────────────────────┐
                                      ▼                                         ▼
                       ┌───────────────────────────┐              ┌───────────────────────────┐
                       │ VectorIndex::add           │              │ Database::insertChunk     │
                       │ (FAISS HNSW, in-memory)    │              │ (SQLite, per-job txn)     │
                       └───────────────────────────┘              └───────────────────────────┘
                                                           │
                                                           ▼
                                              job status → COMPLETED

Meanwhile, client polls:  GET /api/jobs?job_id=... → PipelineStatusPoller → UI stage indicators

Once COMPLETED, client calls POST /api/report:

┌─────────────────────┐                                 ┌──────────────────────────────┐
│ ReportClient         │ ──────────────────────────────▶│ HttpServer::handleReport      │
└─────────────────────┘                                 │  (QtConcurrent::run)          │
                                                          └──────────────┬───────────────┘
                                                                         │ 1. hardcoded RAG query
                                                                         ▼
                                                     LlamaEngine::generateEmbedding(query)
                                                                         │
                                                                         ▼
                                                        VectorIndex::search(top 4) → chunk ids
                                                                         │
                                                                         ▼
                                                   Database::getChunkByVectorId → assembled context
                                                                         │
                                                                         ▼
                                           build ChatML prompt (Qwen Instruct format, fixed JSON schema)
                                                                         │
                                                                         ▼
                                        LlamaEngine::streamCompletion (_genMutex, up to 512 tokens)
                                                                         │
                                                                         ▼
                                            parse generated JSON (nlohmann::json)
                                                                         │
                                                                         ▼
                                   QFile(applicationDirPath + "template.html") + Inja render
                                                                         │
                                                                         ▼
                                                       rendered HTML returned to client
                                                                         │
                                                                         ▼
                                      CentralWorkspace.qml WebEngineView renders the HTML
```

## Pipeline stages (reference)

1. **Upload** — `UploadClient::uploadDocument` (qml-client) → `POST /api/ingest`.
2. **Enqueue** — `HttpServer::handleIngest` → `Database::enqueueJob` → row `PENDING`.
3. **Dispatch** — `JobQueue::pollNextJob`, ticked every 1500ms, one job in flight at a time
   (`_isProcessing` atomic), handed to `QThreadPool::globalInstance()`.
4. **Parse/OCR/Chunk** — `DocumentProcessor::processPdf`: PDFium native text extraction, Tesseract
   OCR fallback per page, 1500-char chunks with no overlap.
5. **Embed** — `LlamaEngine::generateEmbedding` per chunk, serialized by `_embdMutex`.
6. **Index + persist** — `VectorIndex::add` (FAISS HNSW, in-memory only) + `Database::insertChunk`
   (SQLite), one transaction per document.
7. **Status poll** — client polls `GET /api/jobs`; `PipelineStatusPoller` drives the pipeline-stage
   UI.
8. **Report generation** — `HttpServer::handleReport`: fixed RAG query → embed → FAISS top-4 search
   → assemble context from SQLite → build hardcoded Qwen ChatML prompt →
   `LlamaEngine::streamCompletion` (serialized by `_genMutex`, not actually streamed to the HTTP
   client despite the name) → parse JSON → render `template.html` via Inja.
9. **Render** — `CentralWorkspace.qml`'s `WebEngineView` loads the returned HTML.

## Remaining CPU/memory optimization opportunities (no-GPU target)

Model load/unload, phase-scoped residency, and thread-count tuning are **done** (see
`IMPLEMENTATION_PLAN.md` Phase 1). Remaining items:

### Serialize-by-mutex concurrency bottleneck
`_genMutex` and `_embdMutex` each guard a single `llama_context`, so every embedding call and every
generation call are fully serialized server-wide, regardless of request source (ingestion, chat,
report). Somewhat inherent to running a single CPU-bound model instance; load/unload reduces the
practical impact (you can't run embedding and generation concurrently if only one model is resident
at a time) but doesn't eliminate the serialization itself.

### Context size vs. memory
`n_ctx = 4096` with `n_batch = n_ubatch = 4096` allocates KV-cache and batch buffers sized for full-
context batches on every load. If actual prompts (RAG context + report JSON schema instructions)
are well under 4096 tokens in practice, a smaller `n_batch`/`n_ubatch` reduces peak memory without
changing `n_ctx` capacity — worth measuring actual prompt token counts before tuning this.

### Ingestion-side hotspots
- OCR fallback allocates a full-resolution RGBA bitmap per page, freed immediately after — fine for
  isolated pages, but for high-DPI/large PDFs this is a sizable transient per-page allocation with
  no downscaling. A capped render resolution for OCR would cut peak transient memory with no
  quality loss for typical scanned reports.
- Chunking has no overlap — a quality issue more than a perf one, but worth fixing alongside the
  above since it affects RAG retrieval quality (sentences/metrics split across chunk boundaries).

### Other durability/perf notes worth tracking
- FAISS index is in-memory only with no persistence — a server restart silently drops all vector
  search capability until documents are re-ingested, even though raw chunk text survives in
  SQLite. Ties into Phase 3's embedding-persistence work.
- `streamCompletion` is not actually streamed to the HTTP client (full response buffered, then
  returned) — doesn't affect CPU/memory footprint, but affects perceived latency on a slow,
  GPU-less box where a 7B model's full generation can take a while.

## CMake hygiene — still outstanding

See `IMPLEMENTATION_PLAN.md` Phase 0 for the checklist. Still true as of this review:

- **Floating `GIT_TAG master` for llama.cpp** (root `CMakeLists.txt`) — the only non-pinned
  dependency in the whole file; every other `FetchContent_Declare` uses a fixed tag/version. A
  floating `master` means builds are not reproducible over time and a silent upstream API/behavior
  change can break the build or alter inference behavior without any change on this side.
- **No GPU backend flags set for llama.cpp** — nothing explicitly disables CUDA/Vulkan detection
  for llama.cpp (contrast with FAISS, which explicitly sets `FAISS_ENABLE_GPU OFF`).
- **`winget install` invoked during `cmake configure`** — if NASM isn't found, the configure step
  shells out to `winget` to install it system-wide, with no check on the process's exit code and no
  sandboxing. Mutates the host machine as a side effect of running `cmake`, breaks fully
  offline/hermetic builds. Prefer failing fast with a clear message, or vendoring a portable NASM
  binary via `FetchContent`/`file(DOWNLOAD)`.
- **Generated `FindMKL.cmake` written into the source tree** (`cmake/FindMKL.cmake`) on every
  configure — risks accidental commit, means `cmake/` isn't a clean versioned module directory.
  Should write to `${CMAKE_BINARY_DIR}/cmake` instead.
- **`eng.traineddata` downloaded into the source tree** (`${CMAKE_SOURCE_DIR}/eng.traineddata`) —
  same class of issue, plus the `file(DOWNLOAD ...)` call has no `EXPECTED_HASH`, so an interrupted
  or tampered download silently produces a corrupt file that fails confusingly at OCR runtime.
- **No explicit `CMAKE_BUILD_TYPE` default** — for single-config generators (Makefiles, Ninja), a
  plain `cmake -B build` with no `-DCMAKE_BUILD_TYPE` produces an unoptimized build by default,
  directly relevant to the CPU-bound performance goal.
- **No LTO / `/MP` parallel-build flags configured** — for a project this dependency-heavy
  (llama.cpp, FAISS, Tesseract, Leptonica, zlib, libjpeg-turbo, libpng all built from source),
  enabling `CMAKE_INTERPROCEDURAL_OPTIMIZATION` for Release and MSVC's `/MP` would improve
  build-time and (for LTO) potentially runtime performance of hot inference/vector-search paths.
- **Implicit Windows-only assumptions without a guard** — downstream files assume Windows
  unconditionally in several places. An explicit `if(NOT WIN32) message(FATAL_ERROR ...)` near the
  top would make this a documented constraint instead of an implicit one.
- (Low priority) `PUBLIC` vs `PRIVATE` link/include cleanup in `src/libs/CMakeLists.txt` and
  `src/libs/logging/CMakeLists.txt` — module boundaries currently implicit in which headers happen
  to get `#include`d rather than enforced by the build graph.

## Database design — remaining durability/correctness work

Indices, FK enforcement, and cascade delete are **done** (Phase 0). Remaining:

### Embeddings are never persisted; FAISS index is pure RAM
`document_chunks` stores the chunk's **text** and a `vector_id` integer, but never the embedding
vector itself. The only place the actual `std::vector<float>` embedding lives is inside
`faiss::IndexHNSWFlat` in `VectorIndex`, which is constructed empty at process start with no
`write_index`/`read_index` calls anywhere. A server restart leaves `document_chunks` fully
populated with text but the FAISS index empty — every chunk becomes unreachable by vector search
until its parent job is re-ingested from scratch.

**Fix direction** (now scoped per-job per D4, see `IMPLEMENTATION_PLAN.md` Phase 3): add an
`embedding BLOB` column (raw `float[384]` buffer) and an `embedding_model TEXT` column (provenance
— see below) to `document_chunks`; write both during ingestion; rebuild a job's FAISS partition
on-demand from SQLite when that job is reopened, rather than a whole-index rebuild at process start.

### The `vector_id` encoding is a fragile, redundant layer
`vector_id = jobId * 1,000,000 + chunkIndex` exists purely to synthesize a globally-unique integer
ID for FAISS, because FAISS doesn't generate its own. But `document_chunks` already has its own
`id INTEGER PRIMARY KEY AUTOINCREMENT` which is already globally unique by construction. Two
issues: a silent correctness risk if any single document produces ≥1,000,000 chunks (collides with
the next job's encoded ids), and it's solving a problem SQLite already solved. Use
`document_chunks.id` directly as the FAISS id once retrieval is job-scoped (Phase 3) and no longer
needs the encoding to disambiguate jobs.

### No embedding/model provenance tracked
`kEmbeddingDim = 384` is a hardcoded constant matching the current `bge-small-en-v1.5` model's
output dimension, but nothing in the schema records *which* embedding model (and dimension)
produced a given chunk's vector. If the embedding model is ever swapped, old rows' embeddings
become silently incompatible with new ones. Add an `embedding_model TEXT` column (alongside the
`embedding` BLOB column above) populated from `ServerConfig`'s configured model path/name at insert
time, so a mismatch becomes detectable instead of silent.

### RAG chunk fetch is N round-trips instead of one
`handleReport` gets up to 4 vector ids back from `VectorIndex::search`, then calls
`Database::getChunkByVectorId` once per id in a loop — still true as of this review. Batch into a
single `WHERE id IN (...)` query (now using `document_chunks.id` directly once Phase 3 lands)
returning a map keyed by id, so the caller can still assemble context in the FAISS result's
relevance order.

### Summary of remaining schema/query changes

```sql
-- document_chunks gains durability + provenance (Phase 3):
ALTER TABLE document_chunks ADD COLUMN embedding BLOB;
ALTER TABLE document_chunks ADD COLUMN embedding_model TEXT;
```
Plus the code-level changes: drop the `jobId * 1,000,000 + i` encoding in favor of
`document_chunks.id` as the FAISS id directly; batch `getChunkByVectorId` into one `IN (...)` query
for the RAG top-k fetch; add a per-job rebuild step that replays stored embeddings into that job's
`VectorIndex` partition on demand.

There's already a versioned migration mechanism (`PRAGMA user_version`), so the above maps cleanly
onto a new migration entry when Phase 3 implementation starts.

## UI mock review (reference — implemented)

`RightPanel.qml` was split into three focused, reusable components matching the original mock:
`DocumentsPanel.qml` (list + Add button), `SystemStatusPanel.qml` (flat label stack, now wired to
live data per Phase 1), `StatusLogPanel.qml` (status log list). `RightPanel.qml` is a thin
composition of the three. `Main.qml`, `LeftSidebar.qml`, and `CentralWorkspace.qml` match the mock.
Three unused QML files from an earlier iteration (`FilePickerBar.qml`, `PipelineStageIndicator.qml`,
`ReportView.qml`) were deleted as dead code.
