# Security Policy

Security commitments, reporting protocols, and threat boundaries for **Carbon Engine**.

## 1. Supported Versions

| Version Branch | Status | Maintenance Details |
|---|---|---|
| **1.x (Current master)** | Supported | Active security patches, dependency fixes, and vulnerability remediation |
| **< 1.0** | Unsupported | Legacy development checkpoints; no security patches |

## 2. Reporting a Vulnerability

Please report all security vulnerabilities privately. Do not open public GitHub issues for sensitive security defects.

| Channel | Method | Use Case |
|---|---|---|
| **GitHub Advisory (Preferred)** | Open **Security** tab → Click **Report a vulnerability** | Encrypted private vulnerability disclosure |
| **Direct Email** | `carbon.esgreports@gmail.com` | Alternative private communication channel |

### Response Timelines

| Milestone | SLA Window | Commitment |
|---|---|---|
| **Initial Acknowledgment** | ≤ 48 hours | Confirmation of receipt and assigned maintainer contact |
| **Triage & Reproduction** | ≤ 5 business days | Technical assessment and confirmation of severity |
| **Remediation & Patch** | Mutually agreed | Security patch verification and coordinated advisory release |

### Required Vulnerability Information
* **Vulnerability Description**: Summary of flaw and potential impact.
* **Proof of Concept**: Step-by-step reproduction instructions or test document triggering the issue.
* **Environment Profile**: Operating system, architecture, compiler version, and GPU driver profile.
* **Logs & Traces**: Sanitized crash dump, call stack, or stderr log excerpt.

## 3. Security Commitments & Attack Surface Scope

| Scope Area | Security Policy | Threat Mitigation |
|---|---|---|
| **Air-Gapped Telemetry** | Zero outbound network calls | Strictly local LLM inference and local vector storage; UPSI data leakage prevented |
| **File Parser Safety** | Bounds checks on untrusted buffers | Sandboxed memory management across PDFium, Tesseract, and Leptonica parsers |
| **Model Ingestion (GGUF)**| Strict tensor verification | Model weights loaded via llama.cpp with buffer validation against malformed GGUF files |
| **Database Durability** | Parameterized SQL queries only | Zero dynamic SQL concatenation; SQLite WAL mode with atomic commits |
| **Process Enforcement** | Enforced launcher boundaries | Windows daemon execution restricted to tray supervisor or registered Windows Service |

* **Zero Cloud Exposure**: Any Pull Request introducing remote analytics, external fonts, or telemetry without explicit opt-in is rejected.
* **Data Confinement**: Document embeddings, text chunks, and disclosure facts remain strictly on the local filesystem.
