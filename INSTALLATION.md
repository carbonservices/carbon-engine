# Installation Guide

Where Carbon Engine files are installed, how folders are structured, and where user data is kept.

## 1. File Locations by Setup

| Item | Built from Source | Windows Installer | Linux Package |
|---|---|---|---|
| **App Folder** | Project directory | `C:\Program Files\Carbon ESG` | `/usr/` |
| **Programs** | `__Builds/<arch>/bin/` | `bin/` | `/usr/bin/` |
| **Templates** | `src/libs/common/templates/` | `templates/` | `/usr/share/carbon-esg/templates/` |
| **OCR Data** | `tessdata/` | `bin/tessdata/` | `/usr/share/tessdata/` |
| **Shortcuts** | Run directly from build folder | Start Menu shortcuts | `/usr/share/applications/` |

## 2. Windows Installation

* **Installer**: Run `CarbonESG-<version>-Windows-x64.exe`.
* **Default Directory**: `C:\Program Files\Carbon ESG`.
* **Installed Applications**:
  * `carbon-tray.exe`: System tray manager to start, stop, and monitor the engine.
  * `carbon-engine-client.exe`: Desktop workspace to upload documents, review disclosures, and export reports.
  * `carbon-engine.exe`: Background engine (managed by tray or Windows Service).
* **Start Menu**: Adds shortcuts to open the desktop workspace and tray companion.

## 3. Linux Installation

* **Package**: Install the `.deb` package using `sudo dpkg -i carbon-esg_*.deb`.
* **Programs**: Installs `carbon-engine` and `carbon-engine-client` into `/usr/bin/`.
* **Desktop Launcher**: Adds an application shortcut to the system app launcher with icon.
* **Service**: Can be started as a systemd service (`systemctl start carbon-engine`).

## 4. Built from Source (Development)

* **Repository Layout**: Everything stays self-contained inside the cloned project folder.
* **Build Outputs**: Executables are compiled into `__Builds/<arch>/bin/`.
* **Resource Sync**: Build scripts automatically place required templates and OCR files alongside the binaries.
* **Running Locally**: Start `carbon-tray.exe` from the build directory to manage the engine, then open `carbon-engine-client.exe`.

## 5. User Data & Storage

Models, reports, logs, and database files are always stored in your personal `Documents` folder across all platforms:

| Item | Location | Purpose |
|---|---|---|
| **AI Models** | `Documents/Carbon ESG/models/` | AI model files (`.gguf`) used for extraction |
| **Database** | `Documents/Carbon ESG/data/ce-database.sqlite` | Stores project data and extracted disclosures |
| **Logs** | `Documents/Carbon ESG/logs/` | Daily log files for troubleshooting |

* **Clean System**: Nothing is written to your project root or `Program Files` during normal use.
* **Easy Backup**: Backing up the `Documents/Carbon ESG` folder preserves all your data and settings.
