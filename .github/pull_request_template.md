## Summary of Changes

* **What**: Concise description of changes introduced.
* **Why**: Rationale or problem resolved.
* **Issue Reference**: Closes # / Fixes #

## Type of Change

| Selection | Category | Description |
|---|---|---|
| [ ] | **Feature** | New functionality or regulatory taxonomy addition |
| [ ] | **Bug Fix** | Fix for defect, crash, or calculation anomaly |
| [ ] | **Refactoring** | Code quality or architecture improvement with no behavior change |
| [ ] | **HATEOAS / API** | REST route, payload, or hypermedia control update |
| [ ] | **Packaging** | CMake, NSIS installer, or Linux Debian packaging change |
| [ ] | **Documentation** | Documentation, specification, or diagram update |

## Verification & Testing

| Verification Step | Command / Evidence | Status |
|---|---|---|
| **Clean Build** | `cmake --build __Builds/x64/_build --config Release` | [ ] Passed |
| **All Targets** | Built `carbon-engine`, `carbon-engine-client`, `carbon-tray` | [ ] Passed |
| **Code Formatting** | Ran project `clang-format` on touched files | [ ] Passed |
| **Air-Gapped Policy** | Confirmed zero external network calls / telemetry | [ ] Verified |
| **Managed Launch** | Verified `carbon-engine` starts via tray/service only | [ ] Verified |

## Testing Environment

* **Operating System**: Windows 11 (64-bit) / Ubuntu 22.04
* **Toolchain**: MSVC 2022 (v143) / GCC 11+
* **Qt Version**: Qt 6.10.x
* **Hardware Profile**: CPU / GPU (VRAM)

## Visual Changes (UI Only)

* Paste screenshots or screen recordings showing desktop client changes here (or specify N/A).

