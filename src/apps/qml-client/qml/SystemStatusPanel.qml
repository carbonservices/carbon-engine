import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CarbonEngineClient

Rectangle {
    Layout.fillWidth: true
    Layout.preferredHeight: 220
    color: appWindow.bgCard
    radius: 8
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
    property string engineStatus: poller.engineStatus
    property string databaseStatus: poller.databaseStatus
    property string tokenRate: poller.tokenRate

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Label { text: "System status"; font.bold: true; font.pixelSize: 14; color: appWindow.textMain }
        Rectangle { Layout.fillWidth: true; height: 1; color: appWindow.borderCol }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Repeater {
                model: [
                    { "label": "Job queue", "value": jobQueue },
                    { "label": "CPU", "value": cpuLoad },
                    { "label": "Memory", "value": memoryUsage },
                    { "label": "GPU0", "value": gpuLoad },
                    { "label": "Status", "value": engineStatus },
                    { "label": "Database", "value": databaseStatus },
                    { "label": "Tokens", "value": tokenRate }
                ]
                delegate: RowLayout {
                    Layout.fillWidth: true
                    Label { text: modelData.label; color: appWindow.textMuted; font.pixelSize: 12 }
                    Item { Layout.fillWidth: true }
                    Label { text: modelData.value; color: appWindow.textMain; font.pixelSize: 12; font.bold: true }
                }
            }
        }
    }
}
