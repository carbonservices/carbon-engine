import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    Layout.fillWidth: true
    Layout.preferredHeight: 220
    color: appWindow.bgCard
    radius: 8
    border.color: appWindow.borderCol
    border.width: 1
    clip: true

    property string jobQueue: "2/4"
    property string cpuLoad: "15%"
    property string memoryUsage: "97%"
    property string gpuLoad: "12%"
    property string engineStatus: "Inferencing"
    property string databaseStatus: "Reading"
    property string tokenRate: "20 TOPS"

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