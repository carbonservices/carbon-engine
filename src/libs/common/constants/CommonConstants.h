#pragma once

#include <QString>

namespace JobStatus {
inline constexpr const char* Pending = "PENDING";
inline constexpr const char* Processing = "PROCESSING";
inline constexpr const char* Completed = "COMPLETED";
inline constexpr const char* Failed = "FAILED";
inline constexpr const char* Cancelled = "CANCELLED";
}

namespace ExtractionStatus {
inline constexpr const char* NotStarted = "NOT_STARTED";
inline constexpr const char* Extracting = "EXTRACTING";
inline constexpr const char* ReadyForReview = "READY_FOR_REVIEW";
inline constexpr const char* Completed = "COMPLETED";
inline constexpr const char* Accepted = "ACCEPTED";
inline constexpr const char* Failed = "FAILED";
inline constexpr const char* Cancelled = "CANCELLED";
}

namespace ReportSectionStatus {
inline constexpr const char* Draft = "DRAFT";
inline constexpr const char* Updated = "UPDATED";
inline constexpr const char* Failed = "FAILED";
}

namespace EngineMode {
inline constexpr const char* Offline = "OFFLINE";
}

namespace Models {
inline constexpr const char* LocalQwenName = "qwen2.5-7b-instruct";
inline constexpr const char* LocalQwenFile = "models/qwen2.5-7b-instruct-q4_k_m.gguf";
inline constexpr const char* LocalGemmaName = "gemma-2-2b-it";
inline constexpr const char* LocalGemmaFile = "models/gemma-2-2b-it-q4_k_m.gguf";
inline constexpr const char* LocalBgeEmbeddingFile = "models/bge-small-en-v1.5-f16.gguf";
}

namespace ApiRoutes {
inline constexpr const char* Ingest = "/api/ingest";
inline constexpr const char* Jobs = "/api/jobs";
inline constexpr const char* JobCancel = "/api/jobs/cancel";
inline constexpr const char* System = "/api/system";
inline constexpr const char* ExtractionStart = "/api/extraction/start";
inline constexpr const char* ExtractionStatus = "/api/extraction/status";
inline constexpr const char* ExtractionCancel = "/api/extraction/cancel";
inline constexpr const char* ExtractionSections = "/api/extraction/sections";
inline constexpr const char* ExtractionAccept = "/api/extraction/accept";
inline constexpr const char* ReportExport = "/api/report/export";
inline constexpr const char* Reports = "/api/reports";
inline constexpr const char* Documents = "/api/documents";
inline constexpr const char* SchemaBrsr = "/api/schema/brsr";
}

namespace ServerDefaults {
inline constexpr quint16 DefaultPort = 8080;
inline constexpr const char* DefaultDatabaseName = "ce-database";
inline constexpr int DefaultEmbeddingDim = 384;
}

namespace AppInfo {
#ifdef CARBON_ENGINE_VERSION
inline constexpr const char* Version = CARBON_ENGINE_VERSION;
#else
inline constexpr const char* Version = "1.0.0";
#endif
inline constexpr const char* OrganizationName = "Carbon Services";
inline constexpr const char* OrganizationDomain = "carbonservices.io";
inline constexpr const char* ServerAppName = "Carbon Engine";
inline constexpr const char* ClientAppName = "Carbon ESG";
inline constexpr const char* TrayAppName = "Carbon ESG Service Manager";

inline constexpr const char* DaemonMutexName = "CarbonEngine_Daemon_Mutex";
inline constexpr const char* TrayMutexName = "CarbonEngine_Tray_Mutex";

inline constexpr const char* ServerExecutableName = "carbon-engine.exe";
inline constexpr const char* ClientExecutableName = "carbon-engine-client.exe";
inline constexpr const char* TrayExecutableName = "carbon-tray.exe";
inline constexpr const char* IconFileName = "app.ico";
inline constexpr const char* LogoFileName = "logo.png";
}

