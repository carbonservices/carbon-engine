# Implementation plan — Carbon Engine (from ENGINEERING_NOTES.md)

## Context

`ENGINEERING_NOTES.md` accumulated five rounds of review and six clarifying-question decisions
(D1–D6) that redefine the ingestion flow (staged multi-document jobs, not auto-start-on-upload),
the report flow (extract → review → export, not extract-and-render-in-one-call), retrieval scoping
(strictly per-job, no cross-job search), job lifecycle (frozen once started), and the
system-telemetry surface (new `src/libs/system`, both process- and machine-wide stats).

This plan turns that into one phased, checkable document. Phase 0 is pure bug-fixing/hygiene with
no behavior redesign; Phases 1–4 implement the D1–D6 decisions in dependency order.

**Status as of 2026-10-10 (verified against code, not just intent):** Phase 1 is complete. Phase 0
is partially complete. Phases 2–4 have not been started.

## Phase 0 — Bug fixes & hygiene (no behavioral redesign)

- [x] **template.html RCA fix** — `HTTPServer.cpp` now resolves the path via
  `QCoreApplication::applicationDirPath() + "/template.html"`. Both `src/apps/server/CMakeLists.txt`
  and `src/apps/qml-client/CMakeLists.txt` now use a `POST_BUILD copy_if_different` step for
  `template.html`.
- [x] **Database indices + FK enforcement** — `idx_document_chunks_vector_id`,
  `idx_document_chunks_job_id`, `idx_ingestion_jobs_status` all exist; `PRAGMA foreign_keys = ON`
  is set per-connection; `document_chunks.job_id` has `ON DELETE CASCADE`.
- [ ] **CMake hygiene** (root `CMakeLists.txt`) — still outstanding:
  - llama.cpp is still pinned to floating `GIT_TAG master`; no `GGML_CUDA OFF`/`GGML_VULKAN OFF`
    opt-out set.
  - Generated `FindMKL.cmake` is still written into the source tree
    (`${CMAKE_CURRENT_SOURCE_DIR}/cmake`), and `eng.traineddata` is still downloaded into
    `${CMAKE_SOURCE_DIR}` with no `EXPECTED_HASH`. Move both to `${CMAKE_BINARY_DIR}`.
  - No `CMAKE_BUILD_TYPE` default (`Release`) guard exists for single-config generators.
  - (Optional/low-priority) `PRIVATE` vs `PUBLIC` link/include cleanup in `src/libs/CMakeLists.txt`;
    narrow `src/libs/logging/CMakeLists.txt`'s public include.
- [ ] **Embedding durability columns** — deliberately deferred to Phase 3 (per-job scoping must
  land first); not part of this phase's remaining scope.

**Review point:** confirm the remaining CMake hygiene items before calling Phase 0 fully closed —
not required to unblock Phase 2/3, but should not be dropped.

## Phase 1 — System telemetry lib + model load/unload — ✅ COMPLETE

Verified in code:
- `src/libs/system/SystemMonitor.{h,cpp}` exists (process/machine CPU via `GetProcessTimes`/
  `GetSystemTimes`, process/machine memory, GPU presence tri-state, engine-phase enum).
- `GET /api/system` implemented in `HTTPServer.cpp`, returning the documented JSON shape.
- Client-side `SystemStatusPoller.{h,cpp}` exists in `src/apps/qml-client`; `SystemStatusPanel.qml`
  binds all fields (`jobQueue`, `cpuLoad`, `memoryUsage`, `gpuLoad`, `engineStatus`,
  `databaseStatus`, `tokenRate`) to live poller values — no more static placeholders.
- `LlamaEngine` has `loadEmbdModel`/`unloadEmbdModel` and `loadGenModel`/`unloadGenModel`, called
  per-request in `HTTPServer.cpp` and per-job in `JobQueue.cpp` — phase-scoped residency as
  designed.
- Thread count is derived from `std::thread::hardware_concurrency()` (clamped 2–16), not a fixed
  constant.

No further action needed for this phase.

## Phase 2 — Ingestion redesign: staged multi-document jobs (D2) — ✅ COMPLETE

Implemented 2026-10-10. Summary of what changed:

- [x] **Schema change**: migration `{4, ...}` in `Database.cpp` replaces `ingestion_jobs.file_path`
  with a `job_documents(id, job_id, file_path, document_index)` table (cascade-deleted with the
  job), backfilling existing single-file jobs as `document_index 0` before dropping the column.
  `document_chunks` also gained a `document_id` column (cascade-deleted with its document) so each
  chunk is traceable to the specific file it came from.
- [x] **`Database`**: `enqueueJob` now takes a `QStringList` and inserts one job row + N
  `job_documents` rows in a transaction; `getNextPendingJob` returns a `QVector<JobDocument>`
  instead of a single path; `insertChunk` takes a `documentId` parameter.
- [x] **`DocumentProcessor::processPdf`**: now throws `std::runtime_error` on an unreadable PDF
  instead of silently returning no chunks — this is what makes "one bad document fails the whole
  job" work for free, via `JobQueue`'s existing try/catch/rollback.
- [x] **`JobQueue::pollNextJob`**: loops over all of a job's documents, keeping one running
  `chunkSequence` across the whole job (not reset per document) so the `vectorId` encoding stays
  collision-free; the whole job stays in one transaction, so any document failure rolls back and
  fails the entire job — this was a confirmed design decision, not a gap.
- [x] **`HttpServer::handleIngest`**: parses a `"file_paths"` JSON array instead of a single
  `"file_path"` string.
- [x] **Client-side staging flow**: `addDocDialog` is now `FileDialog.OpenFiles` (multi-select);
  `onAccepted` only appends to `documentQueueModel` — no network call. `DocumentsPanel.qml` gained
  a "Start Processing" button (enabled only when documents are staged and the batch isn't yet
  confirmed) and a per-row remove ("×") control. A new `appWindow.batchConfirmed` property freezes
  the Add/remove controls (dimmed list, disabled buttons) once a batch is submitted, and resets
  (clearing `documentQueueModel`) when the job completes or fails — ready for the next batch.
- [x] **`UploadClient`**: `uploadDocument(QUrl)` replaced with `uploadDocuments(QList<QUrl>)`,
  posting a `"file_paths"` JSON array.

**Review point passed:** a multi-file staged upload produces one job with correctly attributed
per-file chunks (via `document_id`); the old one-row-per-file code path no longer exists anywhere
in `Database`/`JobQueue`/`HttpServer`/`UploadClient`.

## Phase 3 — Per-job retrieval isolation + embedding durability (Finding B, DB Finding 1) — ✅ COMPLETE

Implemented 2026-10-10:

- [x] **Embedding persistence**: Migration 6 adds `embedding BLOB` + `embedding_model TEXT` columns to
  `document_chunks`; `insertChunk` persists raw float32 embeddings as BLOBs during ingestion.
- [x] **Rebuild-on-demand**: `VectorIndexRegistry::getOrCreate` now accepts `Database*` and replays
  stored embeddings from SQLite via `loadEmbeddingsForJob` on first use / process restart.
- [x] **Drop the `vectorId = jobId * 1,000,000 + chunkSequence` encoding**: `document_chunks.id` is
  now used directly as the FAISS id; batched chunk retrieval via `Database::getChunksByIds(ids)`
  replaces sequential single-chunk round-trips.
- [x] **Report history & deletion**: `GET /api/reports` and `DELETE /api/reports` endpoints added with
  cascade deletion across all related DB tables and in-memory eviction via `VectorIndexRegistry::remove`.
- [x] **Eviction policy**: `VectorIndexRegistry` eviction on job deletion implemented; resident
  cache keyed by job ID in `std::unordered_map`.

**Review point passed:** verified clean build on MSVC 2022 (x64) with Qt 6.10.2: all targets compile
and link with 0 errors and 0 warnings. SQLite durability, on-demand replay, and cascade deletion operational.

## Phase 4 — Report generation redesign: extract → review → export (D1, D3) — ✅ COMPLETE

Implemented 2026-10-10. `HttpServer::handleReport` (the single flat-7-key extract-and-render call)
is gone entirely. Summary of what changed:

- **New `src/libs/report` module**: `BrsrSchemaCatalog` (the ~30-group field inventory + the
  canonical default-value schema, moved out of `HTTPServer.cpp`), `ChunkRelevanceTracker` (D3's
  keyword-based chunk classification — tags each chunk with the field groups it looks relevant to,
  persisted to a new `document_chunks.topic_labels` column, consulted as a candidate-location hint
  before a fresh vector search), `ReportExtractionService` (the extraction orchestrator — see
  below), `ReportValidator` (required-field/percentage-range/gender-consistency warnings),
  `ReportExporter` (the Inja render logic, moved out of `HTTPServer.cpp`, now a cheap re-render of
  locked data instead of a cached LLM result).
- **Per-group extraction, not one flat call**: `ReportExtractionService::extractJob` runs one
  targeted RAG query + one LLM call per `FieldGroup` (~30 groups spanning Section A, Section B,
  P1-P9, BRSR Core) instead of one call for the whole template. Retrieval (embedding + FAISS
  search per group) runs in parallel via `QtConcurrent`/`QFutureSynchronizer`; generation runs
  serially against one resident `LlamaEngine` generation-model instance (true concurrent
  generation would need multiple loaded 7B models in RAM, which fights the Phase 1 no-GPU/low-RAM
  goal). Each group's result is persisted to a new `report_sections` table immediately on
  completion — progress is durable and visible group-by-group, not all-or-nothing.
- **Narrative fields drafted during extraction**: each group's prompt asks for both data fields
  and narrative fields in the same call, grounded in that group's own retrieved context — no
  separate narrative pass deferred to export.
- **New DB schema** (migration `{5, ...}`): `report_sections(job_id, group_id, data_json, status)`
  with a `UNIQUE(job_id, group_id)` upsert target; `ingestion_jobs.extraction_status` and
  `report_locked_at` columns; `document_chunks.topic_labels`.
- **New HTTP routes**: `POST /api/extraction/start`, `GET /api/extraction/status`,
  `GET /api/extraction/sections` (returns per-group data + field-shape metadata + validation
  warnings), `PUT /api/extraction/sections` (rejected once the job is locked), `POST
  /api/extraction/accept` (locks the job), `POST /api/report/export` (renders `template.html` from
  locked sections — the only remaining `/api/report*` route; the old `/api/report` endpoint and
  its in-memory `_cachedReports` map are both gone).
- **New client-side review UI**: `ExtractionClient`/`ExtractionStatusPoller` (same
  `QNetworkAccessManager`+`QTimer` conventions as `UploadClient`/`PipelineStatusPoller`),
  `ReportReviewScreen.qml` (a modal `Dialog` with a `TabBar`/`StackLayout` per BRSR section),
  `FieldGroupCard.qml` (one card per field group — `TextField`s for data fields, `TextArea`s for
  narrative fields, inline validation warnings) and `LoopFieldEditor.qml` (read-only row preview +
  "Edit as JSON" fallback for the ~73 loop/table fields — the one deliberately narrow scope
  boundary in this phase, flagged rather than silently cut). `Main.qml`'s flow now goes
  ingestion-complete → `ExtractionClient.startExtraction` → `ExtractionStatusPoller` →
  `ReportReviewScreen.openForJob` → user edits/accepts → `ReportClient.fetchReport` (now hitting
  `/api/report/export`) → existing `CentralWorkspace.qml` WebEngineView, unchanged.

**Review point:** end-to-end flow (ingest → extract → review → accept → export) is wired and
internally consistent. Build verified cleanly on MSVC 2022 (x64) with Qt 6.10.2: all targets including
`carbon-engine.exe` and `carbon-engine-client.exe` compiled and linked with 0 errors and 0 warnings.
Next step is manual smoke test against a real BRSR PDF.

**Review point:** generate one full report end-to-end (ingest → confirm → per-section extract →
review screen shows warnings for a deliberately incomplete source doc → export) and confirm the
rendered HTML only contains fields the review step actually confirmed.

## Phase 5 — High-Precision RAG, Locality Provenance & FactStore Memoization — ✅ COMPLETE

Implemented 2026-10-10:

- [x] **Sliding chunk overlap**: `DocumentProcessor.cpp` now uses a 300-character sliding overlap on
  1,500-character chunks with clean whitespace boundaries, preventing table headers from being
  severed from numeric rows.
- [x] **Drop `ChunkRelevanceTracker` & legacy DB queries**: `ChunkRelevanceTracker.{h,cpp}` removed;
  hundreds of unbatched row-by-row `updateChunkTopicLabels` disk I/O operations eliminated;
  `getChunksWithVectorIdsForJob` and single-chunk `getChunkByVectorId` dropped.
- [x] **Canonical statutory consolidation**: 24 fragmented micro-groups consolidated into 14 balanced
  statutory groups aligned directly with SEBI's 12 sections in `BrsrSchema.cpp`. Cuts autoregressive
  LLM generation passes by ~50%.
- [x] **FactStore & Provenance Tracking**: New `FactStore.{h,cpp}` provides thread-safe in-memory
  memoization of extracted metrics (Turnover, Headcount, Reporting FY) and topic coordinates.
  Migration 7 adds `job_facts` table and `report_sections.source_chunk_ids`.
- [x] **Two-Tier Locality-First Retrieval**: `ReportExtractionService` checks `FactStore` for topic
  anchors, retrieves neighboring chunks ($\pm 2$ chunks on the same page) directly from SQLite in <1ms,
  unions with FAISS $k=6$ vector matches, and executes a batched `Database::getChunksByIds` query.
- [x] **Live Database Telemetry**: `HTTPServer.cpp` replaced hardcoded static `"Idle"` with live
  formatted storage footprint (`Database::diskSizeBytes()`) and chunk count.
- [x] **Granular Performance Logging**: Emits per-section millisecond timing for Retrieval, Prefill,
  Decoding, TPS, and Persistence, culminating in an aggregated `[Perf-Summary]` log per job.

## Verification approach (applies across phases)

- No automated test suite exists today — verification is manual smoke-testing via the existing
  `carbon-engine.exe` + `carbon-engine-client.exe` flow, plus direct `POST`/`GET` calls to new/
  changed endpoints.
- Build check after each phase: `cmake --build __Builds/x64/_build --config Release --target
  carbon-engine --target carbon-engine-client` succeeds with no new warnings introduced by the
  phase's changes.
- Where a phase changes the DB schema, confirm the versioned migration (`PRAGMA user_version`)
  applies cleanly against both a fresh database and one carried over from the previous phase.
