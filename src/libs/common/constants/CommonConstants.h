#pragma once

#include <cstdint>
#include <QString>

struct CoreConstants {
    static constexpr int InvalidJobId = -1;
    static constexpr int NoActiveJobId = -1;
    static constexpr int64_t NoSourceChunkId = -1;
    static constexpr int ModelNotLoaded = -1;
    static constexpr int StepUninitialized = -1;
    static constexpr int OffloadAllLayers = 99; // Sentinel convention passed to llama.cpp to offload 100% of available layers
};

struct JobStatus {
    static constexpr const char* Pending = "PENDING";
    static constexpr const char* Processing = "PROCESSING";
    static constexpr const char* Completed = "COMPLETED";
    static constexpr const char* Failed = "FAILED";
    static constexpr const char* Cancelled = "CANCELLED";
};

struct ExtractionStatus {
    static constexpr const char* NotStarted = "NOT_STARTED";
    static constexpr const char* Extracting = "EXTRACTING";
    static constexpr const char* ReadyForReview = "READY_FOR_REVIEW";
    static constexpr const char* Completed = "COMPLETED";
    static constexpr const char* Accepted = "ACCEPTED";
    static constexpr const char* Failed = "FAILED";
    static constexpr const char* Cancelled = "CANCELLED";
};

struct ReportSectionStatus {
    static constexpr const char* Draft = "DRAFT";
    static constexpr const char* Updated = "UPDATED";
    static constexpr const char* Failed = "FAILED";
};

struct EngineMode {
    static constexpr const char* Offline = "OFFLINE";
};

struct Models {
    static constexpr const char* LocalGemmaName = "gemma-2-2b-it";
    static constexpr const char* LocalGemmaFile = "gemma-2-2b-it-q4_k_m.gguf";
    static constexpr const char* LocalBgeEmbeddingFile = "bge-small-en-v1.5-f16.gguf";
};

struct ApiRoutes {
    static constexpr const char* Root = "/api";
    static constexpr const char* Ingest = "/api/ingest";
    static constexpr const char* Jobs = "/api/jobs";
    static constexpr const char* JobCancel = "/api/jobs/cancel";
    static constexpr const char* System = "/api/system";
    static constexpr const char* Settings = "/api/settings";
    static constexpr const char* ExtractionStart = "/api/extraction/start";
    static constexpr const char* ExtractionStatus = "/api/extraction/status";
    static constexpr const char* ExtractionCancel = "/api/extraction/cancel";
    static constexpr const char* ExtractionSections = "/api/extraction/sections";
    static constexpr const char* ExtractionAccept = "/api/extraction/accept";
    static constexpr const char* ReportExport = "/api/report/export";
    static constexpr const char* Reports = "/api/reports";
    static constexpr const char* Documents = "/api/documents";
    static constexpr const char* SchemaBrsr = "/api/schema/brsr";
};

struct ServerDefaults {
    static constexpr quint16 DefaultPort = 8080;
    static constexpr int DefaultEmbeddingDim = 384;
};

struct AppInfo {
#ifdef CARBON_ENGINE_VERSION
    static constexpr const char* Version = CARBON_ENGINE_VERSION;
#else
    static constexpr const char* Version = "1.0.0";
#endif

#ifdef CARBON_WORKLOAD_TITLE
    static constexpr const char* WorkloadTitle = CARBON_WORKLOAD_TITLE;
#elif defined(CARBON_ENGINE_CUDA)
    static constexpr const char* WorkloadTitle = "GPU Accelerated";
#else
    static constexpr const char* WorkloadTitle = "Non-GPU Workload";
#endif

#ifdef CARBON_WORKLOAD_TAG
    static constexpr const char* WorkloadTag = CARBON_WORKLOAD_TAG;
#elif defined(CARBON_ENGINE_CUDA)
    static constexpr const char* WorkloadTag = "CUDA";
#else
    static constexpr const char* WorkloadTag = "Non-GPU";
#endif

#if defined(CARBON_ENGINE_CUDA) && !defined(CARBON_ENGINE_NON_GPU)
    static constexpr bool IsGpuBuild = true;
#else
    static constexpr bool IsGpuBuild = false;
#endif

    static constexpr const char* OrganizationName = "Carbon Services";
    static constexpr const char* OrganizationDomain = "carbonesg.online";
    static constexpr const char* ServerAppName = "Carbon Engine";
    static constexpr const char* ClientAppName = "Carbon ESG";
    static constexpr const char* WorkspaceAppName = "Carbon ESG Workspace";
    static constexpr const char* TrayAppName = "Carbon ESG Workspace";

    static constexpr const char* DaemonMutexName = "CarbonEngine_Daemon_Mutex";
    static constexpr const char* TrayMutexName = "CarbonEngine_Tray_Mutex";
    static constexpr const char* ClientMutexName = "CarbonEngine_Client_Mutex";

#ifdef _WIN32
    static constexpr const char* ServerExecutableName = "carbon-engine.exe";
    static constexpr const char* ClientExecutableName = "carbon-engine-client.exe";
    static constexpr const char* TrayExecutableName = "carbon-tray.exe";
#else
    static constexpr const char* ServerExecutableName = "carbon-engine";
    static constexpr const char* ClientExecutableName = "carbon-engine-client";
    static constexpr const char* TrayExecutableName = "carbon-tray";
#endif
    static constexpr const char* IconFileName = "app.ico";
    static constexpr const char* LogoFileName = "logo.png";
};

