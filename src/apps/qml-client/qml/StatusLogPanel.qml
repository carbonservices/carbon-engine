import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: appWindow.bgCard
    radius: 8
    border.color: appWindow.borderCol
    border.width: 1
    clip: true

    function scrollToBottom() { statusLogList.positionViewAtEnd() }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Label { text: "STATUS LOG"; font.bold: true; font.pixelSize: 12; color: appWindow.brandAccent }
        Rectangle { Layout.fillWidth: true; height: 1; color: appWindow.borderCol }

        ListView {
            id: statusLogList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: statusLogModel
            spacing: 6
            boundsBehavior: Flickable.StopAtBounds
            delegate: Label {
                width: statusLogList.width
                wrapMode: Text.WordWrap
                text: model.entry
                font.pixelSize: 12
                color: appWindow.textMuted
            }
        }
    }
}