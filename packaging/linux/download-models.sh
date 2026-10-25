#!/usr/bin/env bash
set -euo pipefail

MODELS_DIR="${1:-/usr/share/carbon-esg/models}"
mkdir -p "${MODELS_DIR}"

EMBED_FILE="${MODELS_DIR}/bge-small-en-v1.5-f16.gguf"
EMBED_URL="https://huggingface.co/compendiumlabs/bge-small-en-v1.5-gguf/resolve/main/bge-small-en-v1.5-f16.gguf"

if [ ! -f "${EMBED_FILE}" ]; then
    echo "[1/2] Downloading Embedding Model: bge-small-en-v1.5-f16.gguf [67 MB]..."
    if command -v curl >/dev/null 2>&1; then
        curl -f -L --progress-bar -o "${EMBED_FILE}" "${EMBED_URL}"
    elif command -v wget >/dev/null 2>&1; then
        wget -O "${EMBED_FILE}" "${EMBED_URL}"
    else
        echo "[ERROR] Neither curl nor wget found. Cannot download embedding model." >&2
        exit 1
    fi
else
    echo "[1/2] Embedding Model already present."
fi

LLM_FILE="${MODELS_DIR}/gemma-2-2b-it-q4_k_m.gguf"
LLM_URL="https://huggingface.co/bartowski/gemma-2-2b-it-GGUF/resolve/main/gemma-2-2b-it-Q4_K_M.gguf"

if [ ! -f "${LLM_FILE}" ]; then
    echo "[2/2] Downloading Generation Model: gemma-2-2b-it-q4_k_m.gguf [1.71 GB]..."
    if command -v curl >/dev/null 2>&1; then
        curl -f -L --progress-bar -o "${LLM_FILE}" "${LLM_URL}"
    elif command -v wget >/dev/null 2>&1; then
        wget -O "${LLM_FILE}" "${LLM_URL}"
    else
        echo "[ERROR] Neither curl nor wget found. Cannot download LLM model." >&2
        exit 1
    fi
else
    echo "[2/2] Generation Model already present."
fi

chmod 644 "${MODELS_DIR}"/*.gguf 2>/dev/null || true
echo "Model setup completed successfully."
exit 0

