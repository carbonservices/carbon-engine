import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CarbonEngineClient

Rectangle {
    Layout.fillWidth: true
    Layout.preferredHeight: 305
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
        Label { text: label; color: Theme.textMuted; font.pixelSize: 11 }
        Item { Layout.fillWidth: true }
        Label {
            text: value
            color: valueColor
            font.pixelSize: 11
            font.family: "Consolas, monospace"
            elide: Text.ElideRight
            Layout.maximumWidth: 195
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: "SYSTEM STATUS"
                font.bold: true
                font.pixelSize: 11
                color: Theme.textMuted
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

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5

            StatusRow {
                label: "Mode"
                value: (!poller || !poller.isConnected) ? "Disconnected" : "Offline (Air-Gapped)"
                valueColor: (!poller || !poller.isConnected) ? Theme.dangerColor : Theme.textBright
            }
            StatusRow {
                label: "Model"
                value: poller ? poller.activeModel : "—"
                valueColor: (!poller || !poller.isConnected) ? Theme.textMuted : Theme.textBright
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
                valueColor: (!poller || !poller.isConnected) ? Theme.dangerColor : ((poller && poller.engineStatus === "Processing") ? Theme.brandAccent : Theme.textBright)
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
