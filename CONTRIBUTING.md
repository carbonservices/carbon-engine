# Contributing to Carbon Engine

Thank you for your interest in contributing to **Carbon Engine**! We welcome contributions from software developers, machine learning engineers, and ESG/sustainability domain specialists.

Whether you are improving low-level C++ inference kernels, expanding statutory disclosure taxonomies, or enhancing the desktop user experience, your contributions help make sustainable reporting transparent, private, and accessible.

---

## 💡 Contribution Philosophy: Discuss Before Coding

To make sure your time and effort are well spent, **please file an issue in the project repository and discuss your proposal before opening a Pull Request (PR)**.

- **New Features & Architectural Changes**: Open an issue detailing your use case, architectural approach, and rationale. This allows maintainers and the community to align on design decisions, ensure consistency with the air-gapped architecture, and avoid duplicate effort.
- **Discussions on Implementation**: Use the issue thread to agree on technical details (data structures, API design, threading model) before writing code.
- **Small Fixes**: Minor bug fixes, documentation typos, or small build corrections do not require prior discussion and can be submitted directly as PRs.

---

## 🐛 Reporting Bugs

A great bug report helps us diagnose and resolve issues quickly without back-and-forth guessing. When filing a bug report, please provide the following details:

### 1. Observed Behavior
Describe clearly what happened. Include:
- Exact error messages, stack traces, or exception codes.
- Relevant log output from `log/<Module>_<date>.log` or terminal stdout/stderr.
- Screenshots or screen recordings if the issue involves the desktop UI.

### 2. Expected Behavior
Explain what you expected to happen instead according to the documentation or regulatory requirements.

### 3. Setup Constraints & Environment Profile
Carbon Engine interacts directly with local hardware and native C++ libraries. Please provide your setup constraints:
- **Operating System**: Windows 10/11 (64-bit), Ubuntu 22.04, etc.
- **CPU & Architecture**: Processor model, core count, and AVX2 support.
- **System Memory**: Total physical RAM installed and free RAM.
- **GPU & Acceleration**: GPU model (e.g., NVIDIA RTX 3060), VRAM size, and CUDA driver version (if applicable).
- **Software Toolchain**:
  - Compiler: MSVC 2022 (v143), GCC 11+, or Clang 14+
  - Qt version: Qt 6.10 or newer
  - CMake version: CMake 3.24 or newer
- **Models Used**: Model names and quantization levels (e.g., `qwen2.5-7b-instruct-q4_k_m.gguf`, `bge-small-en-v1.5-f16.gguf`).
- **Input Document Profile**: Approximate page count, file size, and whether the document contains digital vector text or scanned image pages.

### 4. Steps to Reproduce
Provide a numbered step-by-step sequence to reproduce the problem from a clean state.

---

## 🛠️ C++20 Standards & Best Practices

All engine code in `src/libs` and `src/apps` follows modern C++20 standards. Please adhere to the following principles:

### 1. Resource Management & RAII
- Rely on RAII (Resource Acquisition Is Initialization) for all resource lifetimes.
- Never use manual `new` and `delete`. Use `std::make_unique` or `std::make_shared`.
- Use `std::unique_ptr` by default for exclusive ownership. Use `std::shared_ptr` only when ownership is genuinely shared across components.
- Wrap external C handles (PDFium objects, Leptonica images, Tesseract instances) in RAII wrappers or smart pointers with custom deleters.

### 2. Const-Correctness & Immutability
- Mark variables, member functions, and parameters `const` whenever they are not intended to be mutated.
- Pass complex types by `const Type&` or `std::string_view` / `std::span` to avoid unnecessary allocations.

### 3. Thread Safety & Concurrency
- The engine runs background workers for document ingestion and statutory extraction concurrently with the UI.
- Protect shared mutable state with appropriate synchronization primitives (`std::mutex`, `std::lock_guard`, `std::unique_lock`, or `std::atomic`).
- Avoid holding locks across long-running inference passes or blocking I/O calls.
- In Qt QML client code, never block the GUI main thread. Perform network requests and heavy computations asynchronously.

### 4. Memory Safety & Boundary Checking
- Perform explicit boundary checks when parsing untrusted binary data, tokens, or PDF streams.
- Avoid raw pointer arithmetic.
- Sanitize JSON strings and handle potential formatting anomalies gracefully.

### 5. Code Formatting & Naming Conventions
- Format code according to the repository's `.clang-format` configuration.
- Follow standard Qt and modern C++ conventions:
  - Classes and structs: `PascalCase` (e.g., `ReportExporter`, `VectorIndex`)
  - Member functions and variables: `camelCase` (e.g., `exportJob`, `activeReportContent`)
  - Private member variables: Prefix with an underscore or descriptive naming (e.g., `_dbPath`, `_catalog`)
  - Constants: `kPascalCase` (e.g., `kDatabaseFilePath`)
- Do not leave trailing whitespace or inconsistent line endings.

---

## 🚀 Pull Request Best Practices

When submitting a Pull Request, follow these guidelines to facilitate a prompt and thorough review:

### 1. Branch Naming & Commits
- Create descriptive branch names:
  - `feature/add-excel-ingestion`
  - `fix/intensity-calculation-overflow`
  - `docs/clarify-build-steps`
- Write clear, imperative commit messages:
  - `fix: handle numeric values safely in report exporter`
  - `feat: add grievance redressal disclosure schema`

### 2. PR Description Requirements
Your pull request description should clearly answer:
- **What**: A concise summary of what changes were made.
- **Why**: The problem being solved or feature being added.
- **Issue Reference**: Link to the issue where this was discussed (e.g., `Fixes #42` or `Addresses #87`).
- **Verification & Testing**:
  - What exact commands were run to build and test the changes.
  - What hardware and OS setup was used during verification.
  - Confirmation that no regressions were introduced in existing pipelines.
- **Visuals**: Screenshots or screen recordings for any desktop UI changes.

---

## 🏗️ Build Verification

Before opening your pull request, verify that the project compiles cleanly without warnings:

### Windows Build Check
```powershell
# 1. Configure
cmake -S . -B __Builds/x64/_build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.10.0/msvc2022_64"

# 2. Build Release targets
cmake --build __Builds/x64/_build --config Release --target carbon-engine --target carbon-engine-client
```

### Smoke Test
1. Launch the server daemon:
   ```powershell
   ./__Builds/x64/bin/carbon-engine.exe
   ```
2. In a separate terminal, launch the desktop client:
   ```powershell
   ./__Builds/x64/bin/carbon-engine-client.exe
   ```
3. Test staging an input document and confirm that extraction and UI components operate smoothly.

---

## 📜 Code of Conduct & License

By contributing to Carbon Engine, you agree that your contributions will be licensed under the project's [GNU General Public License v3.0 (GPLv3)](LICENSE), and you agree to uphold our community standards as described in the [Code of Conduct](CODE_OF_CONDUCT.md).

