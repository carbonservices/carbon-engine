# Security Policy

## Overview

Carbon Engine is built for enterprise ESG and statutory compliance reporting, where annual reports and sustainability binders frequently contain Unpublished Price Sensitive Information (UPSI). A core design commitment of Carbon Engine is that it operates in an air-gapped environment with **zero network telemetry** and **100% on-device inference**.

We take all security considerations seriously, including parser safety, local memory boundaries, database durability, and potential prompt injection risks in statutory parsing pipelines.

---

## Supported Versions

We provide security patches and updates for the following versions:

| Version | Supported |
|---|---|
| 1.x (Current master) | :white_check_mark: |
| < 1.0 | :x: |

---

## Reporting a Security Vulnerability

If you discover a security vulnerability in Carbon Engine, please **do not report it via a public GitHub issue**. Publicly disclosing an issue before a patch is ready puts sensitive user workflows at risk.

### Private Reporting Channels

Please report security concerns privately through one of the following methods:
1. **GitHub Private Vulnerability Reporting**: Go to the **Security** tab of this repository and click **"Report a vulnerability"**.
2. **Email**: Send detailed vulnerability reports to `carbon.esgreports@gmail.com`.

### What to Include in Your Report

To help us triage and resolve the issue quickly, please provide:
- A clear description of the vulnerability and its potential impact.
- Step-by-step reproduction instructions or a minimal proof of concept (PoC).
- Setup constraints and environment details (Operating System, CPU/GPU profile, compiler version, Qt version).
- Any relevant logs, stack traces, or sanitization reports.
- If you have an idea for a remediation or patch, feel free to include it!

### Response Timeline

- **Initial Acknowledgement**: Within 48 hours of receipt.
- **Triage & Assessment**: Within 5 business days, confirming whether the finding is reproducible.
- **Fix & Disclosure Coordination**: We will coordinate with you to verify the fix and agree upon a public release and advisory timeline.

---

## Security Commitments & Scope

### 1. Air-Gapped Operation & Zero Telemetry
Carbon Engine must never make unsolicited outbound network requests. All LLM inference and vector operations are local. Any pull request introducing external network requests without explicit, opt-in user control will be rejected.

### 2. File Parsing & Native Buffer Safety
The engine processes untrusted PDF documents and images using native C++ dependencies (PDFium, Tesseract, Leptonica, libjpeg-turbo, libpng). We welcome security reviews and fuzz testing on:
- Malformed PDF structure parsing.
- Memory corruption or buffer overflows in OCR pre-processing.
- Integer overflows during image decompression.

### 3. Local Model Ingestion (GGUF)
Models are loaded via `llama.cpp`. Malformed GGUF files or tampered tensor weights that cause arbitrary code execution or invalid memory writes fall under high-priority security concerns.

### 4. Database Integrity
The SQLite database stores extracted disclosures and document chunk embeddings. Queries must always use parameterized SQL statements to eliminate injection vectors.

