<p align="center">
  <img src="https://avatars.githubusercontent.com/u/309081559" height="80" alt="Carbon Services Project Logo"/>
</p>

<h1 align="center">Carbon Engine</h1>

<p align="center">
  <strong>Native, Air-Gapped C++ Engine & Desktop Client for Statutory ESG & BRSR Compliance Analytics</strong>
</p>

<p align="center">
  <a href="https://github.com/carbonservices/carbon-engine"><img src="https://img.shields.io/badge/Language-C%2B%2B20-blue.svg" alt="C++20"/></a>
  <a href="https://www.qt.io/"><img src="https://img.shields.io/badge/GUI-Qt%206.10%20(Quick%20%2F%20QML)-41cd52.svg" alt="Qt 6.10"/></a>
  <a href="https://github.com/ggerganov/llama.cpp"><img src="https://img.shields.io/badge/Inference-llama.cpp%20(CUDA%20%2F%20CPU)-orange.svg" alt="llama.cpp"/></a>
  <a href="https://github.com/facebookresearch/faiss"><img src="https://img.shields.io/badge/Vector%20Search-FAISS-blueviolet.svg" alt="FAISS"/></a>
  <a href="https://sqlite.org/"><img src="https://img.shields.io/badge/Storage-SQLite%20WAL-003B57.svg" alt="SQLite WAL"/></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPLv3-blue.svg" alt="License: GPLv3"/></a>
  <a href="#contributing"><img src="https://img.shields.io/badge/PRs-welcome-brightgreen.svg" alt="PRs Welcome"/></a>
</p>

<p align="center">
  <img src="docs/application.png" width="940" alt="Carbon Engine Desktop Client Workspace"/>
  <br/>
  <em>Carbon Engine Desktop Workspace: Real-time local extraction tracking, automated statutory gap cataloging, and live GPU/CPU system telemetry.</em>
</p>

---

## 🌍 Overview

Corporate ESG disclosures, including India's **SEBI BRSR (Business Responsibility & Sustainability Reporting)** framework, **BRSR Core**, and **GRI Standards**, require analyzing hundreds of quantitative metrics and qualitative narratives across 300+ page corporate filings.

Most cloud AI services require uploading sensitive corporate binders to third-party APIs. For public companies and auditors handling Unpublished Price Sensitive Information (UPSI), this creates serious compliance and security risks.

**Carbon Engine** is an **on-device, 100% air-gapped C++ application** with zero cloud dependencies or telemetry:
- Ingests enterprise annual reports and sustainability disclosures locally.
- Runs high-speed vector retrieval and local quantized language models with GPU acceleration.
- Surfaces extracted metrics in a consultant-friendly desktop workspace with spreadsheet-style tabular editing.
- Generates verified, audit-ready HTML and PDF reports with interactive visualizations.

---

## ✨ Key Features

- 🔒 **100% Private & Air-Gapped**: Runs entirely on your local machine with zero data transmitted over the internet.
- 📄 **Fast Document Ingestion**: Native PDF text parsing with automated OCR fallback for scanned pages and images.
- 🧠 **Hardware-Accelerated Local AI**: Powered by `llama.cpp` (CUDA / CPU AVX2) and FAISS vector search:
  - High-precision embeddings for semantic document search.
  - Quantized language models for structured data extraction.
- 🎯 **Targeted Statutory Extraction**: Automatically maps corporate text and financial footnotes into official SEBI BRSR disclosure sections and principles.
- 🛡️ **Fault-Tolerant Processing**: Built-in sanitization and self-healing algorithms safeguard against formatting anomalies or model token truncation.
- 🖥️ **Consultant-Friendly Desktop UI**: Modern Qt Quick application featuring:
  - Official SEBI BRSR human-readable terminology.
  - Spreadsheet-style table editors for complex tabular metrics (Workforce Diversity, Energy, Waste, Emissions).
  - Real-time hardware telemetry (CPU, RAM, GPU VRAM, token generation speed).
  - Missing-metrics queue highlighting unlocated data points for auditor review.
- 📊 **Audit-Ready Report Export**: Generates responsive, offline-ready HTML and PDF reports with integrated charts.

---

## 🏗️ High-Level Architecture

The engine decouples document processing and model inference from the user interface using a lightweight local background daemon:

<p align="center">
  <a href="docs/architecture.drawio" title="Open source Draw.io diagram">
    <img src="docs/architecture.drawio.svg" width="920" alt="Carbon Engine System Architecture"/>
  </a>
  <br/>
  <em>System Architecture. <a href="docs/architecture.drawio">Click here to view or edit in Draw.io</a>.</em>
</p>

- **Ingestion & Indexing**: Documents are parsed, split into sliding overlapping windows, embedded, and indexed into job-isolated vector storage.
- **Statutory Extraction Pipeline**: Relevant document context is retrieved and synthesized by a local LLM into structured disclosure metrics.
- **Desktop Application**: Provides review, calibration, tabular editing, and one-click report generation.

*(Additional technical diagrams: [Database Schema](docs/database_schema.drawio) | [Extraction Pipeline](docs/extraction_pipeline.drawio))*

---

## 💻 System Requirements

### Hardware Requirements

| Component | Minimum | Recommended |
|---|---|---|
| **Processor (CPU)** | x86_64 with AVX2 support (4 cores) | 8+ cores (Intel Core i7/i9 or AMD Ryzen 7/9) |
| **System Memory (RAM)** | 16 GB | 32 GB+ (for processing large 300+ page binders) |
| **Graphics (GPU)** | *(Optional)* CPU inference supported | NVIDIA GPU with 8 GB+ VRAM (RTX 3060, 4060, or better, Compute 7.0+) |
| **Disk Space** | 10 GB free space | SSD with 20 GB+ free space (models + document cache) |

### Software Toolchain

| Tool | Required Version | Purpose |
|---|---|---|
| **Operating System** | Windows 10/11 (64-bit) or Linux (Ubuntu 22.04+) | Host OS |
| **C++ Compiler** | MSVC (Visual Studio 2022) / GCC 11+ / Clang 14+ | C++20 compliant compiler |
| **CMake** | 3.24 or newer | Build system generator |
| **Qt 6** | Qt 6.10 or newer | Core, Quick, QuickControls2, HttpServer, Sql, Concurrent |
| **NASM** | 2.15+ *(Windows)* | Assembler for SIMD image decoding (auto-detected) |
| **CUDA Toolkit** | *(Optional)* CUDA 12.x | GPU acceleration for inference (`GGML_CUDA=ON`) |

---

## 🛠️ Build & Installation

### 1. Clone the Repository

```bash
git clone https://github.com/carbonservices/carbon-engine.git
cd carbon-engine
```

### 2. Download Local Models

Create a `models/` folder and download the recommended GGUF model files:

```powershell
# Create models directory
mkdir models

# Download Embedding Model (BGE-Small-EN-v1.5, ~133 MB)
curl -L -o models/bge-small-en-v1.5-f16.gguf https://huggingface.co/compendiumlabs/bge-small-en-v1.5-gguf/resolve/main/bge-small-en-v1.5-f16.gguf

# Download Extraction Model (Qwen2.5-7B-Instruct-Q4_K_M, ~4.7 GB)
curl -L -o models/qwen2.5-7b-instruct-q4_k_m.gguf https://huggingface.co/Qwen/Qwen2.5-7B-Instruct-GGUF/resolve/main/qwen2.5-7b-instruct-q4_k_m.gguf
```

### 3. Build from Source

#### Windows (Visual Studio 2022 & Qt 6)

```powershell
# Configure build with CMake
cmake -S . -B __Builds/x64/_build -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_PREFIX_PATH="C:/Qt/6.10.0/msvc2022_64"

# Compile engine server and desktop client
cmake --build __Builds/x64/_build --config Release --target carbon-engine --target carbon-engine-client
```

Built binaries will be located in `__Builds/x64/bin/`:
- `carbon-engine.exe` - Local background engine
- `carbon-engine-client.exe` - Qt Quick desktop client

---

## 🚀 Running the Application

### Step 1: Start the Engine Daemon
```powershell
./__Builds/x64/bin/carbon-engine.exe
```
The engine initializes local models and binds locally to `http://localhost:8080`.

### Step 2: Launch the Desktop Client
```powershell
./__Builds/x64/bin/carbon-engine-client.exe
```

### Typical Workflow
1. **Stage Documents**: Drop corporate annual report PDFs into the ingestion panel.
2. **Run Extraction**: Track progress and hardware metrics as the engine extracts disclosures.
3. **Review & Edit**: Inspect extracted fields, verify numbers against source pages, and edit tabular arrays directly in the spreadsheet grid.
4. **Lock & Export**: Finalize disclosures and export audit-ready HTML/PDF reports.

---

## 🤝 Contributing

We welcome contributions from C++ developers, ML engineers, and ESG sustainability specialists!

### How You Can Help
- 📄 **Ingestion**: Expand file format support (`.xlsx`, `.docx`, `.xbrl`) and improve multi-column table extraction.
- 🌐 **Taxonomies**: Add support for global standards like **GRI**, **CSRD / ESRS (EU)**, and **SEC Climate Disclosures**.
- ⚡ **Performance**: Benchmark and optimize inference kernels and context window caching.
- 🖥️ **User Experience**: Enhance desktop visualizations, themes, and accessibility.

### Development Guidelines

Please review our [Contributing Guide](CONTRIBUTING.md) before getting started:
1. File an issue to discuss your approach before opening a PR for new features.
2. Follow modern C++20 conventions, RAII practices, and format code using `.clang-format`.
3. Adhere to our [Code of Conduct](CODE_OF_CONDUCT.md) and [Security Policy](SECURITY.md).
4. Submit a clear Pull Request detailing your changes, setup constraints, and verification steps.

---

## 📄 License

This project is licensed under the **GNU General Public License v3.0 (GPLv3)**. See the [LICENSE](LICENSE) file for details.

---

<p align="center">
  Built with ❤️ for a transparent, sustainable, and data-driven future.
</p>
