pragma Singleton
import QtQuick

QtObject {
    id: theme

    property bool isDarkMode: true

    readonly property color bgBase: isDarkMode ? "#1e1e1e" : "#ffffff"
    readonly property color bgSidebar: isDarkMode ? "#252526" : "#f3f3f3"
    readonly property color bgCard: isDarkMode ? "#252526" : "#ffffff"
    readonly property color bgHeader: isDarkMode ? "#181818" : "#f8f8f8"
    readonly property color bgSubtle: isDarkMode ? "#2a2d2e" : "#e8e8e8"
    readonly property color bgHover: isDarkMode ? "#313131" : "#e0e0e0"
    readonly property color bgActive: isDarkMode ? "#37373d" : "#d0d0d0"
    readonly property color bgInput: isDarkMode ? "#3c3c3c" : "#ffffff"
    readonly property color borderCol: isDarkMode ? "#2d2d2d" : "#e0e0e0"
    readonly property color borderSubtle: isDarkMode ? "#3c3c3c" : "#cccccc"
    readonly property color borderDialog: isDarkMode ? "#4b5058" : "#b8bcc4"
    readonly property color textMain: isDarkMode ? "#cccccc" : "#24292f"
    readonly property color textBright: isDarkMode ? "#ffffff" : "#000000"
    readonly property color textMuted: isDarkMode ? "#858585" : "#6e7781"
    readonly property color brandAccent: isDarkMode ? "#007acc" : "#005fb8"
    readonly property color brandAccentHover: isDarkMode ? "#1177bb" : "#004990"
    readonly property color brandAccentPressed: isDarkMode ? "#0d5c91" : "#003b75"
    readonly property color dangerColor: isDarkMode ? "#e06c75" : "#cf222e"
    readonly property color successColor: isDarkMode ? "#89d185" : "#1a7f37"
    readonly property color warningColor: isDarkMode ? "#cca700" : "#9a6700"

    readonly property color badgeCloudBg: isDarkMode ? "#1a3826" : "#e6f4ea"
    readonly property color badgePillBlueBg: isDarkMode ? "#1e3a5f" : "#dbeafe"
    readonly property color badgePillBlueBorder: isDarkMode ? "#3b82f6" : "#60a5fa"
    readonly property color badgePillBlueText: isDarkMode ? "#93c5fd" : "#1e40af"
    readonly property color badgeWarningBg: isDarkMode ? "#302400" : "#fef3c7"
    readonly property color badgeWarningDarkBg: isDarkMode ? "#291b00" : "#fef9c3"
    readonly property color badgeWarningBorder: isDarkMode ? "#b45309" : "#d97706"
    readonly property color badgeWarningText: isDarkMode ? "#fde047" : "#92400e"
    readonly property color badgeWarningSubtleText: isDarkMode ? "#fef08a" : "#78350f"
    readonly property color badgeWarningPillBg: isDarkMode ? "#451a03" : "#fef3c7"
    readonly property color badgeWarningPillActiveBg: isDarkMode ? "#78350f" : "#fde68a"
    readonly property color badgeDangerBg: isDarkMode ? "#2c1c1f" : "#fef2f2"
    readonly property color badgeDangerBorder: isDarkMode ? "#70383b" : "#fca5a5"
    readonly property color badgeDangerText: isDarkMode ? "#e06c75" : "#b91c1c"
    readonly property color badgeDangerPillBg: isDarkMode ? "#3d1b1e" : "#fee2e2"
    readonly property color badgeNarrativeBg: isDarkMode ? "#1e293b" : "#f1f5f9"
    readonly property color badgeNarrativeBorder: isDarkMode ? "#475569" : "#cbd5e1"
    readonly property color badgeNarrativeText: isDarkMode ? "#94a3b8" : "#475569"
    readonly property color successBtnBg: isDarkMode ? "#16a34a" : "#15803d"
    readonly property color successBtnHover: isDarkMode ? "#15803d" : "#166534"
    readonly property color successBtnPressed: isDarkMode ? "#166534" : "#14532d"
    readonly property color dangerBtnBg: isDarkMode ? "#c53030" : "#b91c1c"
    readonly property color dangerBtnPressed: isDarkMode ? "#9b2c2c" : "#991b1b"
}

