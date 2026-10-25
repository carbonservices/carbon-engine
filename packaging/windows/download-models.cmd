@echo off
setlocal
title Carbon ESG - AI Models Setup

set "ROOT_DIR=%~1"
if "%ROOT_DIR%"=="" set "ROOT_DIR=%~dp0.."
set "MODELS_DIR=%ROOT_DIR%\models"

if not exist "%MODELS_DIR%" mkdir "%MODELS_DIR%"

echo ===============================================================================
echo                 Carbon ESG - Local AI Models Setup
echo ===============================================================================
echo Installation Directory: %ROOT_DIR%
echo Models Directory:       %MODELS_DIR%
echo.

set "EMBED_FILE=%MODELS_DIR%\bge-small-en-v1.5-f16.gguf"
set "EMBED_URL=https://huggingface.co/compendiumlabs/bge-small-en-v1.5-gguf/resolve/main/bge-small-en-v1.5-f16.gguf"

if not exist "%EMBED_FILE%" (
    echo [1/2] Downloading Embedding Model: bge-small-en-v1.5-f16.gguf [67 MB]...
    curl.exe -f -L --progress-bar -o "%EMBED_FILE%" "%EMBED_URL%"
    if errorlevel 1 (
        echo [WARNING] curl failed. Retrying with PowerShell TLS 1.2 download...
        powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; (New-Object Net.WebClient).DownloadFile('%EMBED_URL%', '%EMBED_FILE%')"
    )
) else (
    echo [1/2] Embedding Model already present.
)

set "LLM_FILE=%MODELS_DIR%\gemma-2-2b-it-q4_k_m.gguf"
set "LLM_URL=https://huggingface.co/bartowski/gemma-2-2b-it-GGUF/resolve/main/gemma-2-2b-it-Q4_K_M.gguf"

if not exist "%LLM_FILE%" (
    echo.
    echo [2/2] Downloading Generation Model: gemma-2-2b-it-q4_k_m.gguf [1.71 GB]...
    echo This may take a few minutes depending on your internet connection.
    curl.exe -f -L --progress-bar -o "%LLM_FILE%" "%LLM_URL%"
    if errorlevel 1 (
        echo [WARNING] curl failed. Retrying with PowerShell TLS 1.2 download...
        powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; (New-Object Net.WebClient).DownloadFile('%LLM_URL%', '%LLM_FILE%')"
    )
) else (
    echo.
    echo [2/2] Generation Model already present.
)

echo.
echo ===============================================================================
echo Model setup completed.
echo ===============================================================================
exit /b 0

