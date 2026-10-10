import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CarbonEngineClient

Rectangle {
    Layout.fillWidth: true
    Layout.preferredHeight: 250
    color: appWindow.bgSidebar
    radius: 4
    border.color: appWindow.borderCol
    border.width: 1
    clip: true

    SystemStatusPoller {
        id: poller
    }

    property string jobQueue: poller.jobQueue
    property string cpuLoad: poller.cpuLoad
    property string memoryUsage: poller.memoryUsage
    property string gpuLoad: poller.gpuLoad
    property string vramUsage: poller.vramUsage
    property string gpuOffload: poller.gpuOffload
    property string engineStatus: poller.engineStatus
    property string databaseStatus: poller.databaseStatus
    property string tokenRate: poller.tokenRate

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Label {
            text: "SYSTEM STATUS"
            font.bold: true
            font.pixelSize: 11
            color: appWindow.textMuted
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: appWindow.borderCol }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5

            Repeater {
                model: [
                    { "label": "Job queue", "value": jobQueue },
                    { "label": "CPU", "value": cpuLoad },
                    { "label": "Memory", "value": memoryUsage },
                    { "label": "GPU VRAM", "value": vramUsage },
                    { "label": "Offload", "value": gpuOffload },
                    { "label": "Status", "value": engineStatus },
                    { "label": "Database", "value": databaseStatus },
                    { "label": "Tokens", "value": tokenRate }
                ]
                delegate: RowLayout {
                    Layout.fillWidth: true
                    Label { text: modelData.label; color: appWindow.textMuted; font.pixelSize: 11 }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: modelData.value
                        color: appWindow.textBright
                        font.pixelSize: 11
                        font.family: "Consolas, monospace"
                    }
                }
            }
        }
    }
}
