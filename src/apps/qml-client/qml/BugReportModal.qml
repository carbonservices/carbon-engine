import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CarbonEngineClient

Popup {
    id: root
    modal: true
    focus: true
    anchors.centerIn: parent
    width: Math.min(680, parent.width - 40)
    height: Math.min(640, parent.height - 40)
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var client: (typeof appWindow !== "undefined" && appWindow.gitHubClient) ? appWindow.gitHubClient : (typeof backendGitHubClient !== "undefined" ? backendGitHubClient : null)

    background: Rectangle {
        color: Theme.bgMain
        radius: 8
        border.color: Theme.borderCol
        border.width: 1

        Rectangle {
            z: -1
            anchors.fill: parent
            anchors.margins: -4
            radius: 12
            color: "#000000"
            opacity: 0.3
        }
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: "🐞"
                font.pixelSize: 20
            }

            ColumnLayout {
                spacing: 2
                Label {
                    text: "Submit Bug / Issue Report"
                    font.bold: true
                    font.pixelSize: 15
                    color: Theme.textBright
                }
                Label {
                    text: "Files an issue to GitHub with system specs, model status, and logs."
                    font.pixelSize: 11
                    color: Theme.textMuted
                }
            }

            Item { Layout.fillWidth: true }

            // Close button
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

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderCol }

        // Scrollable Form Area
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: parent.width - 16
                spacing: 12

                // Issue Title
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        text: "Issue Title *"
                        font.pixelSize: 11
                        font.bold: true
                        color: Theme.textMain
                    }

                    TextField {
                        id: titleInput
                        Layout.fillWidth: true
                        placeholderText: "e.g. Model inference timeout on large PDF document"
                        color: Theme.textBright
                        background: Rectangle {
                            color: Theme.bgInput
                            border.color: titleInput.activeFocus ? Theme.brandAccent : Theme.borderSubtle
                            border.width: 1
                            radius: 4
                        }
                    }
                }

                // Severity Selection
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Label {
                        text: "Severity:"
                        font.pixelSize: 11
                        font.bold: true
                        color: Theme.textMain
                    }

                    ComboBox {
                        id: severityCombo
                        Layout.preferredWidth: 160
                        model: ["Low", "Normal", "High", "Critical"]
                        currentIndex: 1
                        background: Rectangle {
                            color: Theme.bgInput
                            border.color: Theme.borderSubtle
                            border.width: 1
                            radius: 4
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                // Description
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        text: "Bug Description *"
                        font.pixelSize: 11
                        font.bold: true
                        color: Theme.textMain
                    }

                    TextArea {
                        id: descInput
                        Layout.fillWidth: true
                        Layout.preferredHeight: 80
                        placeholderText: "Please describe what happened, what you expected, and any error message displayed."
                        wrapMode: Text.WordWrap
                        color: Theme.textBright
                        background: Rectangle {
                            color: Theme.bgInput
                            border.color: descInput.activeFocus ? Theme.brandAccent : Theme.borderSubtle
                            border.width: 1
                            radius: 4
                        }
                    }
                }

                // Steps to Reproduce
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        text: "Steps to Reproduce (Optional)"
                        font.pixelSize: 11
                        font.bold: true
                        color: Theme.textMain
                    }

                    TextArea {
                        id: stepsInput
                        Layout.fillWidth: true
                        Layout.preferredHeight: 60
                        placeholderText: "1. Upload PDF report\n2. Click Process Documents\n3. Wait for section extraction..."
                        wrapMode: Text.WordWrap
                        color: Theme.textBright
                        background: Rectangle {
                            color: Theme.bgInput
                            border.color: stepsInput.activeFocus ? Theme.brandAccent : Theme.borderSubtle
                            border.width: 1
                            radius: 4
                        }
                    }
                }

                // Diagnostic Attachments Checkboxes
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: attachmentsCol.implicitHeight + 16
                    color: Theme.bgSubtle
                    border.color: Theme.borderSubtle
                    border.width: 1
                    radius: 4

                    ColumnLayout {
                        id: attachmentsCol
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 6

                        Label {
                            text: "Diagnostic Attachments"
                            font.bold: true
                            font.pixelSize: 11
                            color: Theme.textBright
                        }

                        CheckBox {
                            id: sysInfoCheck
                            text: "Attach System & Hardware Info (OS, CPU cores, RAM, GPU, VRAM)"
                            checked: true
                        }

                        CheckBox {
                            id: modelInfoCheck
                            text: "Attach AI Model & Runtime Status (Gemma 2B, BGE Embeddings, OCR)"
                            checked: true
                        }

                        CheckBox {
                            id: logsCheck
                            text: "Attach Recent Application Logs (last 80 lines, secrets redacted)"
                            checked: true
                        }
                    }
                }

                // Diagnostics Preview Toggle
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Button {
                        text: previewArea.visible ? "Hide Diagnostics Preview ▲" : "Preview Diagnostics Preview ▼"
                        flat: true
                        Layout.preferredHeight: 26
                        onClicked: previewArea.visible = !previewArea.visible
                    }

                    Button {
                        text: "📋 Copy Diagnostics"
                        flat: true
                        Layout.preferredHeight: 26
                        onClicked: {
                            if (root.client) {
                                var preview = root.client.getDiagnosticPreview(logsCheck.checked, sysInfoCheck.checked || modelInfoCheck.checked)
                                root.client.copyToClipboard(preview)
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                TextArea {
                    id: previewArea
                    visible: false
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    readOnly: true
                    text: (root.client && visible) ? root.client.getDiagnosticPreview(logsCheck.checked, sysInfoCheck.checked || modelInfoCheck.checked) : ""
                    font.family: "Consolas, Courier New, monospace"
                    font.pixelSize: 10
                    wrapMode: Text.NoWrap
                    background: Rectangle {
                        color: Theme.isDarkMode ? "#141414" : "#f1f1f1"
                        border.color: Theme.borderSubtle
                        border.width: 1
                        radius: 3
                    }
                }

                // GitHub Token Field
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    RowLayout {
                        spacing: 6
                        Label {
                            text: "GitHub Personal Access Token"
                            font.pixelSize: 11
                            font.bold: true
                            color: Theme.textMain
                        }
                        Label {
                            text: "(Optional — enables direct in-app submission)"
                            font.pixelSize: 10
                            color: Theme.textMuted
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        TextField {
                            id: tokenInput
                            Layout.fillWidth: true
                            echoMode: showTokenCheck.checked ? TextInput.Normal : TextInput.Password
                            placeholderText: "ghp_xxxxxxxxxxxxxxxxxxxx (needs 'repo' or 'issues: write')"
                            text: root.client ? root.client.githubToken : ""
                            color: Theme.textBright
                            background: Rectangle {
                                color: Theme.bgInput
                                border.color: tokenInput.activeFocus ? Theme.brandAccent : Theme.borderSubtle
                                border.width: 1
                                radius: 4
                            }
                            onTextChanged: {
                                if (root.client && text !== root.client.githubToken) {
                                    root.client.setGithubToken(text)
                                }
                            }
                        }

                        CheckBox {
                            id: showTokenCheck
                            text: "Show"
                        }
                    }
                }

                // Feedback & Result Messages
                Rectangle {
                    visible: root.client && (root.client.hasError || root.client.statusMessage !== "")
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    radius: 4
                    color: (root.client && root.client.hasError) ? Theme.badgeDangerBg : Theme.badgeSuccessBg
                    border.color: (root.client && root.client.hasError) ? Theme.badgeDangerBorder : Theme.successColor
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8

                        Text {
                            text: (root.client && root.client.hasError) ? "⚠️" : "✅"
                            font.pixelSize: 12
                        }

                        Label {
                            Layout.fillWidth: true
                            text: (root.client && root.client.hasError) ? root.client.errorMessage : (root.client ? root.client.statusMessage : "")
                            color: (root.client && root.client.hasError) ? Theme.badgeDangerText : Theme.successColor
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }

                        Button {
                            visible: root.client && root.client.lastIssueUrl !== ""
                            text: "View Issue"
                            flat: true
                            Layout.preferredHeight: 22
                            onClicked: {
                                if (root.client && root.client.lastIssueUrl !== "") {
                                    Qt.openUrlExternally(root.client.lastIssueUrl)
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderCol }

        // Bottom Actions
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            BusyIndicator {
                running: root.client ? root.client.isSubmittingIssue : false
                visible: running
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
            }

            Label {
                visible: root.client ? root.client.isSubmittingIssue : false
                text: "Uploading issue to GitHub..."
                font.pixelSize: 11
                color: Theme.brandAccent
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Cancel"
                flat: true
                Layout.preferredHeight: 30
                onClicked: root.close()
            }

            // Direct in-app submit or submit via web
            Button {
                text: "🌐 Open on GitHub.com"
                flat: true
                Layout.preferredHeight: 30
                background: Rectangle {
                    color: parent.hovered ? Theme.bgHover : "transparent"
                    border.color: Theme.borderSubtle
                    border.width: 1
                    radius: 4
                }
                onClicked: {
                    if (titleInput.text.trim() === "") {
                        titleInput.forceActiveFocus()
                        return
                    }
                    if (root.client) {
                        root.client.openBugReportInBrowser(
                            titleInput.text,
                            descInput.text,
                            stepsInput.text,
                            severityCombo.currentText,
                            logsCheck.checked,
                            sysInfoCheck.checked || modelInfoCheck.checked
                        )
                    }
                }
            }

            Button {
                text: "Submit to GitHub"
                highlighted: true
                enabled: titleInput.text.trim() !== "" && !(root.client && root.client.isSubmittingIssue)
                Layout.preferredHeight: 30
                background: Rectangle {
                    color: parent.enabled
                        ? (parent.down ? Theme.brandAccentPressed : (parent.hovered ? Theme.brandAccentHover : Theme.brandAccent))
                        : Theme.bgSubtle
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? "#ffffff" : Theme.textMuted
                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    if (root.client) {
                        root.client.submitBugReport(
                            titleInput.text,
                            descInput.text,
                            stepsInput.text,
                            severityCombo.currentText,
                            logsCheck.checked,
                            sysInfoCheck.checked || modelInfoCheck.checked
                        )
                    }
                }
            }
        }
    }
}

