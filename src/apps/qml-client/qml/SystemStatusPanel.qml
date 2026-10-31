import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CarbonEngineClient

Rectangle {
    id: root
    Layout.fillWidth: true
    property real panelHeight: 330
    Layout.preferredHeight: panelHeight
    Layout.minimumHeight: 180
    Layout.maximumHeight: 460
    color: Theme.bgSidebar
    radius: 4
    border.color: Theme.borderCol
    border.width: 1
    clip: true

    property var poller: (typeof appWindow !== "undefined" && appWindow.systemPoller) ? appWindow.systemPoller : null

    component StatusRow: RowLayout {
        property string label: ""
        property string value: ""
        property color valueColor: Theme.textBright
        Layout.fillWidth: true
        spacing: 4
        clip: true

        Label {
            text: label
            color: Theme.textMuted
            font.pixelSize: 11
            elide: Text.ElideRight
            Layout.maximumWidth: Math.max(70, parent ? parent.width * 0.42 : 110)
        }
        Item { Layout.fillWidth: true }
        Label {
            id: valLabel
            text: value
            color: valueColor
            font.pixelSize: 11
            font.family: "Consolas, monospace"
            elide: Text.ElideRight
            Layout.maximumWidth: Math.max(90, parent ? parent.width * 0.55 : 140)
            ToolTip.visible: valMouse.containsMouse && valLabel.truncated
            ToolTip.text: value
            ToolTip.delay: 300
            MouseArea {
                id: valMouse
                anchors.fill: parent
                hoverEnabled: true
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Label {
                text: "SYSTEM STATUS"
                font.bold: true
                font.pixelSize: 11
                color: Theme.textMuted
                elide: Text.ElideRight
            }
            Item { Layout.fillWidth: true }

            Label {
                text: (!poller || !poller.isConnected) ? "DISCONNECTED" : "ONLINE"
                font.bold: true
                font.pixelSize: 10
                color: (!poller || !poller.isConnected) ? Theme.dangerColor : Theme.successColor
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderCol }

        ScrollView {
            id: statusScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: statusScrollView.availableWidth
                spacing: 5

                StatusRow {
                    label: "Mode"
                    value: (!poller || !poller.isConnected) ? "Disconnected" : "Local Daemon"
                    valueColor: (!poller || !poller.isConnected) ? Theme.dangerColor : Theme.textBright
                }
                StatusRow {
                    label: "Workload"
                    value: poller ? poller.workloadTitle : "Non-GPU Workload"
                    valueColor: Theme.brandAccent
                }
                StatusRow {
                    label: "Inference Mode"
                    value: poller ? poller.activeInferenceMode : "Auto (System Specs)"
                    valueColor: (poller && !poller.systemSpecsPrecedence) ? Theme.warningColor : Theme.brandAccent
                }
                StatusRow {
                    label: "Model"
                    value: {
                        if (typeof appWindow !== "undefined" && appWindow.modelDownloader && !appWindow.modelDownloader.modelsReady) {
                            return "Not Downloaded"
                        }
                        return poller ? poller.activeModel : "—"
                    }
                    valueColor: (typeof appWindow !== "undefined" && appWindow.modelDownloader && !appWindow.modelDownloader.modelsReady)
                        ? Theme.warningColor
                        : ((!poller || !poller.isConnected) ? Theme.textMuted : Theme.textBright)
                }
                StatusRow { label: "Job queue"; value: poller ? poller.jobQueue : "—" }
                StatusRow { label: "CPU"; value: poller ? poller.cpuLoad : "—" }
                StatusRow { label: "Memory"; value: poller ? poller.memoryUsage : "—" }
                StatusRow { label: "GPU Load"; value: poller ? poller.gpuLoad : "—" }
                StatusRow { label: "GPU VRAM"; value: poller ? poller.vramUsage : "—" }
                StatusRow { label: "Offload"; value: poller ? poller.gpuOffload : "—" }
                StatusRow {
                    label: "Status"
                    value: poller ? poller.engineStatus : "—"
                    valueColor: (!poller || !poller.isConnected) ? Theme.dangerColor : ((poller && poller.engineStatus !== "Idle" && poller.engineStatus !== "Unknown") ? Theme.brandAccent : Theme.textBright)
                }
                StatusRow {
                    label: "Database"
                    value: poller ? poller.databaseStatus : "—"
                    valueColor: (!poller || !poller.isConnected) ? Theme.dangerColor : Theme.textBright
                }
                StatusRow { label: "Tokens"; value: poller ? poller.tokenRate : "—" }
            }
        }
    }
}
