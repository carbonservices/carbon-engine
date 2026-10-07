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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Label { text: "Add documents"; font.bold: true; font.pixelSize: 14; color: appWindow.textMain }
        Rectangle { Layout.fillWidth: true; height: 1; color: appWindow.borderCol }

        ListView {
            id: queueList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: documentQueueModel
            spacing: 6
            boundsBehavior: Flickable.StopAtBounds
            delegate: Rectangle {
                width: queueList.width
                height: 32
                color: appWindow.bgSubtle
                radius: 6
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 8
                    text: model.fileName
                    elide: Text.ElideMiddle
                    font.pixelSize: 12
                    color: appWindow.textMain
                }
            }
        }

        Button {
            text: "+ Add Document"
            Layout.alignment: Qt.AlignRight
            Layout.preferredHeight: 32
            background: Rectangle { 
                color: parent.down ? Qt.darker(appWindow.brandAccent, 1.2) : appWindow.brandAccent
                radius: 6
            }
            contentItem: Text {
                text: parent.text
                color: "#0a192f"
                font.pixelSize: 12
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: addDocDialog.open()
        }
    }
}