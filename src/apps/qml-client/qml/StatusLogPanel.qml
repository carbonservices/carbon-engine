import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: appWindow.bgSidebar
    radius: 4
    border.color: appWindow.borderCol
    border.width: 1
    clip: true

    function scrollToBottom() { statusLogList.positionViewAtEnd() }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Label {
            text: "STATUS LOG"
            font.bold: true
            font.pixelSize: 11
            color: appWindow.textMuted
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: appWindow.borderCol }

        ListView {
            id: statusLogList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: statusLogModel
            spacing: 4
            boundsBehavior: Flickable.StopAtBounds
            delegate: Label {
                width: statusLogList.width
                wrapMode: Text.WordWrap
                text: model.entry
                font.pixelSize: 11
                font.family: "Consolas, monospace"
                color: appWindow.textMuted
            }
        }
    }
}