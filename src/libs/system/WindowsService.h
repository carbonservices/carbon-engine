#pragma once

#include <QString>
#include <functional>

class WindowsService {
public:
    WindowsService() = delete;

    // Default Service Identification
    static constexpr const wchar_t* kServiceName = L"CarbonEngine";
    static constexpr const wchar_t* kServiceDisplayName = L"Carbon Engine Service";
    static constexpr const wchar_t* kServiceDescription = L"Statutory BRSR ESG inference and report generation engine by Carbon Services.";

    // Inspects argc/argv for service verbs:
    // --service            : Dispatches execution to SCM as a Windows Service
    // --install-service    : Registers the Windows Service with SCM (auto-start)
    // --uninstall-service  : Stops and removes the Windows Service from SCM
    // --start-service      : Starts the registered Windows Service
    // --stop-service       : Stops the active Windows Service
    // Returns true if a service verb was handled (so main() should exit immediately).
    // Returns false if no service verb matched (proceed with normal interactive execution).
    static bool handleServiceCommands(int argc, char* argv[], std::function<int(int, char**)> runDaemonFunc);

    // Programmatic service management methods
    static bool installService(QString& outMessage);
    static bool uninstallService(QString& outMessage);
    static bool startService(QString& outMessage);
    static bool stopService(QString& outMessage);
    static bool isServiceInstalled();
    static bool isServiceRunning();
};

