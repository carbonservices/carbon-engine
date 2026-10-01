import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 12
    Layout.preferredWidth: 320
    Layout.fillHeight: true

    function scrollToBottom() { statusLogList.positionViewAtEnd() }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 220
        color: appWindow.bgCard
        radius: 8
        border.color: appWindow.borderCol
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8
            RowLayout {
                Layout.fillWidth: true
                Label { text: "Add documents"; font.bold: true; font.pixelSize: 14; color: appWindow.textMain }
                Item { Layout.fillWidth: true }
                Button { text: "Add"; Layout.preferredHeight: 32; onClicked: addDocDialog.open() }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: appWindow.borderCol }
            ListView {
                id: queueList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: documentQueueModel
                spacing: 4
                delegate: Rectangle {
                    width: queueList.width
                    height: 28
                    color: appWindow.bgSubtle
                    radius: 4
                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: 6
                        text: model.fileName
                        elide: Text.ElideMiddle
                        font.pixelSize: 12
                        color: appWindow.textMain
                    }
                }
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 210
        color: appWindow.bgCard
        radius: 8
        border.color: appWindow.borderCol
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 6
            Label { text: "System status"; font.bold: true; font.pixelSize: 14; color: appWindow.textMain }
            Rectangle { Layout.fillWidth: true; height: 1; color: appWindow.borderCol }
            GridLayout {
                columns: 2
                columnSpacing: 12
                rowSpacing: 4
                Layout.fillWidth: true
                Label { text: "Job queue:"; color: appWindow.textMuted; font.pixelSize: 12 }
                Label { text: "1 active"; color: appWindow.textMain; font.bold: true; font.pixelSize: 12 }
                Label { text: "CPU Load:"; color: appWindow.textMuted; font.pixelSize: 12 }
                Label { text: "15% (4 threads)"; color: appWindow.textMain; font.pixelSize: 12 }
                Label { text: "Memory:"; color: appWindow.textMuted; font.pixelSize: 12 }
                Label { text: "13.3 / 15.7 GB"; color: appWindow.textMain; font.pixelSize: 12 }
                Label { text: "GPU0 / Compute:"; color: appWindow.textMuted; font.pixelSize: 12 }
                Label { text: "CPU Exclusive"; color: appWindow.textMain; font.pixelSize: 12 }
                Label { text: "Engine Status:"; color: appWindow.textMuted; font.pixelSize: 12 }
                Label { text: "Ready"; color: appWindow.brandAccent; font.bold: true; font.pixelSize: 12 }
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        color: appWindow.bgCard
        radius: 8
        border.color: appWindow.borderCol
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 6
            Label { text: "STATUS LOG"; font.bold: true; font.pixelSize: 12; color: appWindow.brandAccent }
            Rectangle { Layout.fillWidth: true; height: 1; color: appWindow.borderCol }
            ListView {
                id: statusLogList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: statusLogModel
                spacing: 4
                delegate: Label {
                    width: statusLogList.width
                    wrapMode: Text.WordWrap
                    text: "• " + model.entry
                    font.pixelSize: 11
                    color: appWindow.textMuted
                }
            }
        }
    }
}