import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CarbonEngineClient

Popup {
    id: root
    modal: true
    focus: true
    anchors.centerIn: parent
    width: Math.min(560, parent.width - 40)
    height: Math.min(540, parent.height - 40)
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var poller: (typeof appWindow !== "undefined" && appWindow.systemPoller) ? appWindow.systemPoller : null
    property var client: (typeof appWindow !== "undefined" && appWindow.gitHubClient) ? appWindow.gitHubClient : (typeof backendGitHubClient !== "undefined" ? backendGitHubClient : null)

    property int currentTab: 0

    background: Rectangle {
        color: Theme.bgMain
        radius: 8
        border.color: Theme.borderCol
        border.width: 1

        // Drop shadow illusion
        Rectangle {
            z: -1
            anchors.fill: parent
            anchors.margins: -4
            radius: 12
            color: "#000000"
            opacity: 0.25
        }
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12

        // 1. Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: "⚙"
                font.pixelSize: 18
                color: Theme.brandAccent
            }

            ColumnLayout {
                spacing: 2
                Label {
                    text: root.currentTab === 0 ? "Hardware & Compute Settings" : "Updates, Feedback & GitHub"
                    font.bold: true
                    font.pixelSize: 14
                    color: Theme.textBright
                }
                Label {
                    text: root.currentTab === 0
                        ? "Configure hardware inference acceleration and testing overrides."
                        : "Check for new releases, manage tokens, and submit diagnostics."
                    font.pixelSize: 11
                    color: Theme.textMuted
                }
            }

            Item { Layout.fillWidth: true }

            // Close (X) button
            Rectangle {
                width: 24; height: 24; radius: 3
                color: closeMouse.containsMouse ? Theme.bgHover : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    font.pixelSize: 12
                    color: closeMouse.containsMouse ? Theme.textBright : Theme.textMuted
                }

                MouseArea {
                    id: closeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }
        }

        // Tab Selector Row
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                radius: 4
                color: root.currentTab === 0 ? Theme.brandAccent : Theme.bgSubtle
                border.color: root.currentTab === 0 ? Theme.brandAccent : Theme.borderSubtle
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "⚡ Compute & Hardware"
                    font.pixelSize: 11
                    font.bold: true
                    color: root.currentTab === 0 ? "#ffffff" : Theme.textMain
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.currentTab = 0
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                radius: 4
                color: root.currentTab === 1 ? Theme.brandAccent : Theme.bgSubtle
                border.color: root.currentTab === 1 ? Theme.brandAccent : Theme.borderSubtle
                border.width: 1

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 6

                    Text {
                        text: "🔄 Updates & GitHub"
                        font.pixelSize: 11
                        font.bold: true
                        color: root.currentTab === 1 ? "#ffffff" : Theme.textMain
                    }

                    Rectangle {
                        visible: root.client && root.client.updateAvailable
                        width: 8; height: 8; radius: 4
                        color: "#3b82f6"
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.currentTab = 1
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderCol }

        // TAB 0: Compute & Hardware Settings
        ColumnLayout {
            visible: root.currentTab === 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // Active Mode Indicator Banner
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                radius: 4
                color: (!poller || poller.systemSpecsPrecedence) ? Theme.bgSubtle : Theme.badgeWarningBg
                border.color: (!poller || poller.systemSpecsPrecedence) ? Theme.borderSubtle : Theme.warningColor
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    Text {
                        text: (!poller || poller.systemSpecsPrecedence) ? "⚡" : "⚠️"
                        font.pixelSize: 14
                        color: (!poller || poller.systemSpecsPrecedence) ? Theme.brandAccent : Theme.warningColor
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        Label {
                            text: {
                                if (!poller) return "System Mode"
                                if (poller.systemSpecsPrecedence) return "System Auto Specs (Active)"
                                if (poller.forceCpu) return "Testing Override: Forced CPU Mode Active"
                                if (poller.forceGpu) return "Testing Override: Forced GPU Mode Active"
                                return "System Specs Active"
                            }
                            font.bold: true
                            font.pixelSize: 11
                            color: Theme.textBright
                        }

                        Label {
                            text: poller ? ("Mode: " + poller.activeInferenceMode + " • Device: " + (poller.gpuPresent ? poller.gpuName : "CPU")) : "System Hardware"
                            font.pixelSize: 10
                            color: Theme.textMuted
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            // Checkbox 1: Force CPU
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 64
                radius: 4
                color: cpuCheck.checked ? Theme.bgSubtle : "transparent"
                border.color: cpuCheck.checked ? Theme.brandAccent : Theme.borderSubtle
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 10

                    CheckBox {
                        id: cpuCheck
                        checked: poller ? poller.forceCpu : false
                        onToggled: if (poller) poller.setForceCpuMode(checked)
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Label {
                            text: "Force CPU Mode (Disable GPU Offloading)"
                            font.bold: true
                            font.pixelSize: 11
                            color: Theme.textBright
                        }
                        Label {
                            text: "Forces LLM and embeddings to execute 100% on CPU."
                            font.pixelSize: 10
                            color: Theme.textMuted
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            // Checkbox 2: Force GPU
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 64
                radius: 4
                color: gpuCheck.checked ? Theme.bgSubtle : "transparent"
                border.color: gpuCheck.checked ? Theme.brandAccent : Theme.borderSubtle
                border.width: 1
                opacity: (poller && poller.gpuPresent && poller.workloadTag !== "Non-GPU") ? 1.0 : 0.5

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 10

                    CheckBox {
                        id: gpuCheck
                        enabled: (poller && poller.gpuPresent && poller.workloadTag !== "Non-GPU") ? true : false
                        checked: (poller && poller.workloadTag !== "Non-GPU") ? poller.forceGpu : false
                        onToggled: if (poller) poller.setForceGpuMode(checked)
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Label {
                            text: "Force Dedicated GPU Mode (100% Offload)"
                            font.bold: true
                            font.pixelSize: 11
                            color: Theme.textBright
                        }
                        Label {
                            text: (poller && poller.gpuPresent) ? ("Offloads all layers to: " + poller.gpuName) : "No dedicated GPU detected."
                            font.pixelSize: 10
                            color: Theme.textMuted
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }

        // TAB 1: Updates, Feedback & GitHub Settings
        ScrollView {
            visible: root.currentTab === 1
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: parent.width - 12
                spacing: 12

                // Release Status Box
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 68
                    radius: 4
                    color: (root.client && root.client.updateAvailable) ? (Theme.isDarkMode ? "#13233a" : "#ebf5ff") : Theme.bgSubtle
                    border.color: (root.client && root.client.updateAvailable) ? Theme.brandAccent : Theme.borderSubtle
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Text {
                            text: (root.client && root.client.updateAvailable) ? "🚀" : "📦"
                            font.pixelSize: 18
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Label {
                                text: (root.client && root.client.updateAvailable)
                                    ? ("New Update Available: v" + root.client.latestVersion)
                                    : ("Carbon ESG is Up to Date (v" + (root.client ? root.client.currentVersion : "") + ")")
                                font.bold: true
                                font.pixelSize: 12
                                color: Theme.textBright
                            }

                            Label {
                                text: (root.client && root.client.updateAvailable)
                                    ? "A newer build is ready on GitHub. Install to get latest improvements."
                                    : ("Installed version: v" + (root.client ? root.client.currentVersion : ""))
                                font.pixelSize: 10
                                color: Theme.textMuted
                            }
                        }

                        Button {
                            text: (root.client && root.client.updateAvailable) ? "⬇️ Download" : "Check Now"
                            highlighted: root.client && root.client.updateAvailable
                            enabled: !(root.client && root.client.isCheckingUpdate)
                            Layout.preferredHeight: 28
                            onClicked: {
                                if (root.client) {
                                    if (root.client.updateAvailable) {
                                        root.client.downloadAndInstallUpdate()
                                    } else {
                                        root.client.checkForUpdates(false)
                                    }
                                }
                            }
                        }
                    }
                }

                // Quick Action Buttons
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Button {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        flat: true
                        background: Rectangle {
                            color: parent.hovered ? Theme.bgHover : Theme.bgSubtle
                            border.color: Theme.borderSubtle
                            border.width: 1
                            radius: 4
                        }
                        contentItem: RowLayout {
                            anchors.centerIn: parent
                            spacing: 6
                            Text { text: "🐞"; font.pixelSize: 13 }
                            Label { text: "File Bug Report"; font.pixelSize: 11; font.bold: true; color: Theme.textBright }
                        }
                        onClicked: {
                            root.close()
                            if (typeof appWindow !== "undefined") appWindow.openBugReportModal()
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        flat: true
                        background: Rectangle {
                            color: parent.hovered ? Theme.bgHover : Theme.bgSubtle
                            border.color: Theme.borderSubtle
                            border.width: 1
                            radius: 4
                        }
                        contentItem: RowLayout {
                            anchors.centerIn: parent
                            spacing: 6
                            Text { text: "⭐"; font.pixelSize: 13 }
                            Label { text: "Give Rating / Feedback"; font.pixelSize: 11; font.bold: true; color: Theme.textBright }
                        }
                        onClicked: {
                            root.close()
                            if (typeof appWindow !== "undefined") appWindow.openFeedbackModal()
                        }
                    }
                }

                // Cloud Diagnostics Proxy (Carbon Web Service)
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    CheckBox {
                        id: useCloudProxyBox
                        text: "Submit in-app via Carbon Cloud Diagnostics (No GitHub login needed)"
                        checked: root.client ? root.client.useCloudProxy : true
                    }

                    Label {
                        visible: useCloudProxyBox.checked
                        text: "Carbon Cloud Service URL"
                        font.pixelSize: 11
                        font.bold: true
                        color: Theme.textMain
                    }

                    TextField {
                        id: cloudProxyUrlInput
                        visible: useCloudProxyBox.checked
                        Layout.fillWidth: true
                        text: root.client ? root.client.cloudProxyUrl : "https://carbonesg.online"
                        placeholderText: "https://carbonesg.online (or local http://127.0.0.1:4173)"
                        color: Theme.textBright
                        background: Rectangle {
                            color: Theme.bgInput
                            border.color: Theme.borderSubtle
                            border.width: 1
                            radius: 4
                        }
                    }
                }

                // GitHub Settings
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        text: "GitHub Repository (Owner / Name)"
                        font.pixelSize: 11
                        font.bold: true
                        color: Theme.textMain
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        TextField {
                            id: repoOwnerInput
                            Layout.preferredWidth: 140
                            text: root.client ? root.client.repoOwner : "carbonservices"
                            color: Theme.textBright
                            background: Rectangle {
                                color: Theme.bgInput
                                border.color: Theme.borderSubtle
                                border.width: 1
                                radius: 4
                            }
                        }

                        Label { text: "/"; font.bold: true; color: Theme.textMuted }

                        TextField {
                            id: repoNameInput
                            Layout.fillWidth: true
                            text: root.client ? root.client.repoName : "carbon-engine"
                            color: Theme.textBright
                            background: Rectangle {
                                color: Theme.bgInput
                                border.color: Theme.borderSubtle
                                border.width: 1
                                radius: 4
                            }
                        }
                    }
                }

                // Personal Access Token
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        text: "GitHub Personal Access Token (Optional)"
                        font.pixelSize: 11
                        font.bold: true
                        color: Theme.textMain
                    }

                    TextField {
                        id: settingsTokenInput
                        Layout.fillWidth: true
                        echoMode: TextInput.Password
                        text: root.client ? root.client.githubToken : ""
                        placeholderText: "ghp_xxxxxxxxxxxxxxxxxxxx (Leave empty to use Cloud Proxy)"
                        color: Theme.textBright
                        background: Rectangle {
                            color: Theme.bgInput
                            border.color: Theme.borderSubtle
                            border.width: 1
                            radius: 4
                        }
                    }
                }

                // Auto-check updates checkbox
                CheckBox {
                    id: autoCheckUpdatesBox
                    text: "Automatically check for new releases on startup"
                    checked: root.client ? root.client.autoCheckUpdates : true
                }

                Button {
                    text: "Save Configuration"
                    Layout.alignment: Qt.AlignRight
                    Layout.preferredHeight: 28
                    onClicked: {
                        if (root.client) {
                            root.client.saveSettings(
                                settingsTokenInput.text,
                                repoOwnerInput.text,
                                repoNameInput.text,
                                autoCheckUpdatesBox.checked,
                                cloudProxyUrlInput.text,
                                useCloudProxyBox.checked
                            )
                        }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderCol }

        // 5. Actions Footer
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                visible: root.currentTab === 0
                text: "Reset to System Specs (Auto)"
                font.pixelSize: 11
                onClicked: if (poller) poller.resetToSystemSpecs()
            }

            Button {
                visible: root.currentTab === 1
                text: "Reset to Defaults"
                font.pixelSize: 11
                onClicked: if (root.client) root.client.resetSettings()
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Close"
                font.pixelSize: 11
                highlighted: true
                onClicked: root.close()
            }
        }
    }
}
