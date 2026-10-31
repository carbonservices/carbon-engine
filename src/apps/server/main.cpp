#include "ServerConfig.h"
#include "api/HTTPServer.h"
#include "db/Database.h"
#include "inference/LlamaEngine.h"
#include "ingestion/DocumentProcessor.h"
#include "logging/Logger.h"
#include "queue/JobQueue.h"
#include "report/BRSRSchema.h"
#include "report/ReportExtractionService.h"
#include "report/ReportExporter.h"
#include "system/SystemMonitor.h"
#include "vector/VectorIndexRegistry.h"
#include "system/SingleInstanceGuard.h"
#include "system/WindowsService.h"

#include <QCoreApplication>
#include <iostream>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

int runServerDaemon(int argc, char *argv[])
{
    SingleInstanceGuard guard(AppInfo::DaemonMutexName);
    if (!guard.isPrimary())
    {
        Logger log(AppInfo::ServerAppName);
        log.critical(QString("Another instance of %1 is already running. Exiting.").arg(AppInfo::ServerAppName));
        std::cerr << "Another instance of " << AppInfo::ServerAppName << " is already running. Exiting." << std::endl;
        return 1;
    }

    QCoreApplication app(argc, argv);
    app.setApplicationName(AppInfo::ServerAppName);
    app.setOrganizationName(AppInfo::OrganizationName);
    app.setOrganizationDomain(AppInfo::OrganizationDomain);
    app.setApplicationVersion(AppInfo::Version);
    Logger log(AppInfo::ServerAppName);

    Database db(kDatabaseFilePath);
    if (!db.init())
    {
        log.critical("Database initialization failed. Exiting.");
        return 1;
    }

    SystemMonitor monitor;
    LlamaEngine llama(kGenerationModelPath, kEmbeddingModelPath, monitor);
    if (!llama.initBackend())
    {
        log.critical("llama.cpp backend initialization failed. Exiting.");
        return 1;
    }

    QObject::connect(&app, &QCoreApplication::aboutToQuit, &llama, &LlamaEngine::shutdown);

    VectorIndexRegistry indexRegistry(kEmbeddingDim, &db);
    DocumentProcessor docProcessor;

    JobQueue jobQueue(db, docProcessor, llama, indexRegistry);
    jobQueue.start(1500);

    BRSRSchemaCatalog schemaCatalog;
    ReportExtractionService extractionService(db, llama, indexRegistry, schemaCatalog, monitor);
    ReportExporter exporter(db, schemaCatalog);

    QObject::connect(&jobQueue, &JobQueue::jobCompleted, &extractionService, &ReportExtractionService::startAsyncExtraction);

    HttpServer server(db, llama, indexRegistry, monitor, schemaCatalog, extractionService, exporter, jobQueue);
    if (!server.listen(kDefaultPort))
    {
        log.critical(QString("Failed to bind HTTP server to port %1.").arg(kDefaultPort));
        return 1;
    }

    log.info(QString("Carbon Engine listening on http://localhost:%1").arg(kDefaultPort));
    return app.exec();
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--version") == 0 || std::strcmp(argv[i], "-v") == 0) {
            std::cout << AppInfo::ServerAppName << " version " << AppInfo::Version << " (" << AppInfo::WorkloadTitle << ")" << std::endl;
            return 0;
        }
        if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            std::cout << "Usage: " << AppInfo::ServerExecutableName << " [options]\n\n"
                      << "Options:\n"
                      << "  --help, -h          Show help message\n"
                      << "  --version, -v       Show version information\n"
#ifdef _WIN32
                      << "  --tray-managed      Internal launch flag used by " << AppInfo::TrayExecutableName << "\n"
                      << "  --service           Run as Windows Service (dispatched by SCM)\n"
                      << "  --install-service   Register as Windows Service\n"
                      << "  --uninstall-service Unregister Windows Service\n"
                      << "  --start-service     Start registered Windows Service\n"
                      << "  --stop-service      Stop registered Windows Service\n"
#else
                      << "  --service           Run as system service\n"
#endif
                      << std::endl;
            return 0;
        }
    }

#ifdef _WIN32
    if (WindowsService::handleServiceCommands(argc, argv, &runServerDaemon)) {
        return 0;
    }

    bool isTrayManaged = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--tray-managed") == 0) {
            isTrayManaged = true;
            break;
        }
    }

    bool isTrayRunning = false;
    if (isTrayManaged) {
        isTrayRunning = SingleInstanceGuard::isRunning(AppInfo::TrayMutexName);
    }

    if (!isTrayManaged || !isTrayRunning) {
        const char* errMsg =
            "======================================================================\n"
            " Carbon Engine: Direct Execution Not Allowed\n"
            "======================================================================\n"
            " To prevent multiple server runs, Carbon Engine cannot be started by\n"
            " directly invoking the executable.\n\n"
            " Carbon Engine is not allowed to run outside tray or as a service.\n\n"
            " Please start the server using the Carbon ESG Tray companion:\n"
            "   -> carbon-tray.exe\n"
            " Or run as a registered Windows Service:\n"
            "   -> carbon-engine.exe --start-service\n"
            "======================================================================\n";

        std::cerr << errMsg << std::endl;

        HWND consoleWnd = GetConsoleWindow();
        DWORD processList[2] = {0};
        DWORD processCount = GetConsoleProcessList(processList, 2);
        if (consoleWnd && processCount <= 1) {
            MessageBoxW(
                NULL,
                L"Direct execution of Carbon Engine is not permitted.\n\n"
                L"Carbon Engine is not allowed to run outside tray or as a service.\n\n"
                L"To prevent multiple unmanaged server runs, please start the server via the Carbon ESG Tray companion (carbon-tray.exe) or through the Windows Service.",
                L"Carbon Engine",
                MB_ICONWARNING | MB_OK
            );
        }
        return 1;
    }

    return runServerDaemon(argc, argv);

#else
    bool isService = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--service") == 0 ||
            std::strcmp(argv[i], "--systemd") == 0 ||
            std::strcmp(argv[i], "--tray-managed") == 0) {
            isService = true;
            break;
        }
    }

    if (std::getenv("INVOCATION_ID") != nullptr || std::getenv("JOURNAL_STREAM") != nullptr) {
        isService = true;
    }

    if (!isService) {
        std::cerr << "======================================================================\n"
                  << " Carbon Engine: Direct Execution Not Allowed\n"
                  << "======================================================================\n"
                  << " Directly running the binary is not permitted.\n\n"
                  << " Carbon Engine is not allowed to run outside tray or as a service.\n\n"
                  << " On Linux, Carbon Engine is enforced to execute via the service\n"
                  << " method only. Please start the server using its system service:\n"
                  << "   -> systemctl start carbon-engine\n"
                  << " Or run explicitly with the service flag:\n"
                  << "   -> carbon-engine --service\n"
                  << "======================================================================\n"
                  << std::endl;
        return 1;
    }

    return runServerDaemon(argc, argv);
#endif
}
