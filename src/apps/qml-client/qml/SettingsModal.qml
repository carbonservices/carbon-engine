import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CarbonEngineClient

Popup {
    id: root
    modal: true
    focus: true
    anchors.centerIn: parent
    width: Math.min(520, parent.width - 40)
    height: Math.min(460, parent.height - 40)
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var poller: (typeof appWindow !== "undefined" && appWindow.systemPoller) ? appWindow.systemPoller : null

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
        spacing: 14

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
                    text: "Hardware & Inference Settings"
                    font.bold: true
                    font.pixelSize: 14
                    color: Theme.textBright
                }
                Label {
                    text: "Toggle compute backends for testing on single-device setups."
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

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderCol }

        // 2. Current Active Mode Indicator Banner
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            radius: 4
            color: (!poller || poller.systemSpecsPrecedence) ? Theme.bgSubtle : Theme.badgeWarningBg
            border.color: (!poller || poller.systemSpecsPrecedence) ? Theme.borderSubtle : Theme.warningColor
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                Text {
                    text: (!poller || poller.systemSpecsPrecedence) ? "✓" : "⚡"
                    font.pixelSize: 14
                    color: (!poller || poller.systemSpecsPrecedence) ? Theme.successColor : Theme.warningColor
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Label {
                        text: "Active Runtime Mode:"
                        font.pixelSize: 10
                        color: Theme.textMuted
                    }
                    Label {
                        text: poller ? poller.activeInferenceMode : "Auto (System Specs)"
                        font.bold: true
                        font.pixelSize: 11
                        color: Theme.textBright
                        elide: Text.ElideRight
                    }
                }
            }
        }

        // 3. Testing Checkbox Controls
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            // Checkbox 1: Force CPU Mode
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 70
                radius: 4
                color: cpuCheck.checked ? Theme.bgSubtle : "transparent"
                border.color: cpuCheck.checked ? Theme.brandAccent : Theme.borderSubtle
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10

                    CheckBox {
                        id: cpuCheck
                        checked: poller ? poller.forceCpu : false
                        onToggled: {
                            if (poller) poller.setForceCpuMode(checked)
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Label {
                            text: "Force CPU Mode (Disable GPU Offloading)"
                            font.bold: true
                            font.pixelSize: 12
                            color: Theme.textBright
                        }
                        Label {
                            text: "Forces LLM and embeddings to execute 100% on CPU. Tests dynamic multi-variant CPU dispatch and instruction set detection."
                            font.pixelSize: 10
                            color: Theme.textMuted
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            // Checkbox 2: Force Dedicated GPU Mode
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 70
                radius: 4
                color: gpuCheck.checked ? Theme.bgSubtle : "transparent"
                border.color: gpuCheck.checked ? Theme.brandAccent : Theme.borderSubtle
                border.width: 1
                opacity: (poller && poller.gpuPresent && poller.workloadTag !== "Non-GPU") ? 1.0 : 0.5

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10

                    CheckBox {
                        id: gpuCheck
                        enabled: (poller && poller.gpuPresent && poller.workloadTag !== "Non-GPU") ? true : false
                        checked: (poller && poller.workloadTag !== "Non-GPU") ? poller.forceGpu : false
                        onToggled: {
                            if (poller) poller.setForceGpuMode(checked)
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Label {
                            text: "Force Dedicated GPU Mode (100% Offload)"
                            font.bold: true
                            font.pixelSize: 12
                            color: Theme.textBright
                        }
                        Label {
                            text: {
                                if (poller && poller.workloadTag === "Non-GPU") {
                                    return "Dedicated GPU acceleration is disabled in this Non-GPU application build."
                                }
                                if (poller && poller.gpuPresent) {
                                    return "Forces full GPU offload (all layers) to dedicated GPU: " + poller.gpuName
                                }
                                return "No dedicated GPU detected or supported on this system."
                            }
                            font.pixelSize: 10
                            color: Theme.textMuted
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }

        // 4. Default Precedence Informational Box
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            radius: 4
            color: Theme.bgSidebar
            border.color: Theme.borderSubtle
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Text {
                    text: "ℹ"
                    font.pixelSize: 12
                    color: Theme.brandAccent
                }
                Label {
                    text: "Default: When no checkboxes are selected, System Specs takes precedence automatically."
                    font.pixelSize: 10
                    color: Theme.textMuted
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }
        }

        Item { Layout.fillHeight: true }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderCol }

        // 5. Actions Footer
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "Reset to System Specs (Auto)"
                font.pixelSize: 11
                onClicked: {
                    if (poller) poller.resetToSystemSpecs()
                }
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

