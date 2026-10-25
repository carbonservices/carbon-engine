import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CarbonEngineClient

Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Theme.bgSidebar
    radius: 4
    border.color: Theme.borderCol
    border.width: 1
    clip: true

    property var logModel: (typeof statusLogModel !== "undefined") ? statusLogModel : null

    function scrollToBottom() { statusLogList.positionViewAtEnd() }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Label {
            text: "STATUS LOG"
            font.bold: true
            font.pixelSize: 11
            color: Theme.textMuted
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderCol }

        ListView {
            id: statusLogList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: logModel
            spacing: 4
            boundsBehavior: Flickable.StopAtBounds
            delegate: Label {
                width: statusLogList.width
                wrapMode: Text.WordWrap
                text: model.entry
                font.pixelSize: 11
                font.family: "Consolas, monospace"
                color: Theme.textMuted
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSubtle; opacity: 0.5 }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Item { Layout.fillWidth: true }

            Label {
                text: "Carbon ESG v" + Qt.application.version + " (" + (typeof appWindow !== "undefined" && appWindow.systemPoller ? appWindow.systemPoller.workloadTitle : "Non-GPU Workload") + " • Qt " + Qt.version + ")"
                font.pixelSize: 9
                color: Theme.textMuted
                opacity: 0.65
            }
        }
    }
}
