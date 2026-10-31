# Contributing to Carbon Engine

Guidelines and technical standards for contributing to **Carbon Engine**.

## 1. Contribution Workflow

| Contribution Type | Required Action | Approval Channel |
|---|---|---|
| **New Features & Architectural Changes** | Open an issue describing use case, architecture, and design | Issue discussion & maintainer sign-off |
| **API & Schema Modifications** | Propose changes to HATEOAS routes, JSON schemas, or BRSR taxonomy | Issue discussion before PR |
| **Bug Fixes & Security Hardening** | Submit PR directly with reproduction steps and test verification | GitHub Pull Request |
| **Documentation & Typo Fixes** | Submit PR directly | GitHub Pull Request |

### Core Principles
* **Discuss Before Coding**: Avoid wasted effort by aligning on architectural decisions in an issue before writing large PRs.
* **Air-Gapped Integrity**: Never introduce remote cloud telemetry or unsolicited outbound network requests.
* **User Data Location**: User models, SQLite database, and logs must strictly reside in `<Documents>/Carbon ESG/`.
* **Managed Execution**: Server must execute strictly under tray management or as a registered system service.

## 2. API Architecture & HATEOAS Standards

Carbon Engine implements a RESTful hypermedia architecture using RFC 8288 / HAL-style `_links`.

### HATEOAS Response Format
All API responses must include a state-aware `_links` object:

```json
{
  "job_id": 42,
  "status": "PROCESSING",
  "_links": {
    "self": { "href": "/api/jobs?job_id=42", "method": "GET", "title": "Check job status" },
    "cancel": { "href": "/api/jobs/cancel", "method": "POST", "title": "Cancel active ingestion" },
    "documents": { "href": "/api/jobs/documents?job_id=42", "method": "GET", "title": "View ingested documents" }
  }
}
```

### Hypermedia Link Matrix

| Relation (`rel`) | Route Pattern | HTTP Verb | Purpose & State Trigger |
|---|---|---|---|
| `self` | `/api/...` | `GET` | Canonical link to current resource |
| `system` | `/api/system` | `GET` | Telemetry, CPU/GPU, VRAM, and daemon status |
| `settings` | `/api/settings` | `GET`, `POST` | Hardware inspection and offline inference test |
| `ingest` | `/api/ingest` | `POST` | Queue documents for text extraction and indexing |
| `jobs` / `status` | `/api/jobs?job_id={id}` | `GET` | Poll ingestion progress and status transitions |
| `cancel` | `/api/jobs/cancel` | `POST` | Abort active ingestion job (`PENDING` or `PROCESSING`) |
| `documents` | `/api/jobs/documents?job_id={id}` | `GET` | Enumerate ingested files and page counts |
| `start_extraction` | `/api/extraction/start` | `POST` | Trigger LLM extraction once ingestion is `COMPLETED` |
| `extraction_status` | `/api/extraction/status?job_id={id}` | `GET` | Poll extraction progress, active group, and missing metrics |
| `cancel_extraction`| `/api/extraction/cancel` | `POST` | Abort active LLM extraction pass |
| `sections` | `/api/extraction/sections?job_id={id}` | `GET` | Retrieve structured BRSR disclosure sections |
| `update_section` | `/api/extraction/sections` | `PUT` | Save human edits to a section (`group_id`, `data`) |
| `accept_report` | `/api/extraction/accept` | `POST` | Lock report disclosures against further edits |
| `export_report` | `/api/report/export` | `POST` | Generate audit-ready HTML report document |
| `reports` | `/api/reports` | `GET` | Enumerate stored reports with embedded `_links` |
| `delete_report` | `/api/reports?job_id={id}` | `DELETE` | Purge report data and vector index |
| `schema` | `/api/schema/brsr` | `GET` | Return full BRSR taxonomy and field metadata |

### API Implementation Rules
* **Dynamic Navigation**: Clients must discover and traverse actions via `_links` relations rather than synthesizing hardcoded endpoints.
* **State Sensitivity**: `_links` must only advertise actions valid for the current state (e.g. `cancel` only during active processing; `start_extraction` only after ingestion succeeds).
* **HTTP Verbs**: Use standard semantics:
  * `GET` for safe, idempotent reads.
  * `POST` for job dispatch and state creation.
  * `PUT` for complete section data replacement.
  * `DELETE` for resource teardown.
* **No Out-of-Band State**: All state transitions must be discoverable via payload links.

## 3. Engineering & Coding Standards

### C++20 Standards

| Area | Rule | Enforcement Pattern |
|---|---|---|
| **Resource Management** | Strict RAII for all lifetimes | No manual `new`/`delete`; use `std::unique_ptr` and `std::make_unique` |
| **External C Handles** | Wrap C API pointers safely | Wrap PDFium, Leptonica, and Tesseract handles in RAII structs with custom deleters |
| **Const-Correctness** | Default to immutable state | Mark variables, member functions, and parameters `const` whenever unmutated |
| **Type Passing** | Avoid unnecessary allocations | Pass non-trivial types by `const Type&`, `std::string_view`, or `std::span` |
| **Concurrency** | Asynchronous worker queues | Protect shared state with `std::mutex` and `std::lock_guard`; never hold locks across LLM inference passes |
| **Memory Safety** | Bounds checking on untrusted data | Validate all binary buffers, PDF streams, and token inputs before processing |
| **Naming Conventions** | Qt / Modern C++ conventions | Types in `PascalCase`, methods/variables in `camelCase`, constants in `kPascalCase` |

### Storage & Runtime Boundaries
* **User Storage Policy**: All user data (models, SQLite database, and diagnostic logs) must strictly reside in `<Documents>/Carbon ESG/`. Refer to [INSTALLATION.md](INSTALLATION.md) for installation and folder details.
* **Process Boundaries**: The backend server must be executed under tray companion supervision or as a registered system service; direct binary execution is prohibited.

### QML & Client Standards
* **Non-Blocking UI**: Perform all HTTP calls and polling asynchronously; never block the main GUI thread.
* **Theme Styling**: Use visual iconography/emoji for theme switching without redundant text labels.
* **Responsive Layouts**: Central workspace and sidebar sections must support split-view resizing without child element overflow.

## 4. Reporting Issues & Bugs

| Field | Description | Example |
|---|---|---|
| **Observed Behavior** | Clear description of failure, stack traces, and error outputs | Error dialog: "Model backend initialization failed" |
| **Expected Behavior** | Expected operation per documentation or specification | Server loads GGUF model and starts listening on port 8080 |
| **Log Excerpt** | Tail from `<Documents>/Carbon ESG/logs/<Module>_<date>.log` | `[2026-10-31 11:00:00.123] [critical] ...` |
| **System Profile** | OS version, CPU, RAM, GPU model, VRAM, and driver | Windows 11 64-bit, AMD Ryzen 7, 32 GB RAM, RTX 3060 12GB |
| **Toolchain Info** | Compiler, Qt version, CMake version | MSVC 2022 (v143), Qt 6.10.2, CMake 3.28 |
| **Reproduction Steps** | Numbered sequence from a clean state | 1. Launch tray 2. Start engine 3. Drop sample.pdf |

## 5. Pull Request Submissions

* **PR Template**: Pull Requests must follow the repository template in [`.github/pull_request_template.md`](.github/pull_request_template.md).
* **Single Responsibility**: Keep pull requests focused on a single feature, fix, or schema update; do not bundle unrelated changes.
* **Verification Criteria**: Ensure all targets build cleanly, `clang-format` passes, and zero external network calls are introduced.

## 6. Build & Smoke Test Verification

* **Build Commands**: Follow the build and compilation instructions in [README.md#build--installation](README.md#build--installation).
* **Smoke Testing**: Verify changes using the runtime launch sequence documented in [README.md#quick-start](README.md#quick-start).

## 7. License & Code of Conduct

* Contributions are licensed under [GNU General Public License v3.0 (GPLv3)](LICENSE).
* Community interactions are governed by our [Code of Conduct](CODE_OF_CONDUCT.md).
