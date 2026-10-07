import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Pane {
    id: root
    signal uploadRequested(url fileUrl)

    background: Rectangle {
        color: appWindow.bgCard
        radius: 12
        border.color: appWindow.borderCol
        border.width: 1
    }

    RowLayout {
        anchors.fill: parent
        spacing: 16

        Button {
            text: "Browse PDF"
            onClicked: fileDialog.open()
            background: Rectangle {
                color: parent.down ? appWindow.bgCard : appWindow.bgSubtle
                radius: 8
                border.color: appWindow.borderCol
                border.width: 1
            }
            contentItem: Text {
                text: parent.text
                color: appWindow.textMain
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.bold: true
                font.pixelSize: 13
            }
        }

        Label {
            id: pathLabel
            text: fileDialog.selectedFile.toString() === "" ? "No file selected..." : fileDialog.selectedFile.toString().replace("file:///", "")
            color: appWindow.textMuted
            elide: Text.ElideMiddle
            Layout.fillWidth: true
            font.pixelSize: 14
        }

        Button {
            text: "Upload & Process"
            enabled: fileDialog.selectedFile.toString().length > 0
            onClicked: root.uploadRequested(fileDialog.selectedFile)
            background: Rectangle {
                color: parent.enabled ? (parent.down ? Qt.darker(appWindow.brandAccent, 1.2) : appWindow.brandAccent) : appWindow.bgSubtle
                radius: 8
            }
            contentItem: Text {
                text: parent.text
                color: parent.enabled ? "#0a192f" : appWindow.textMuted
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.bold: true
                font.pixelSize: 13
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Select a document"
        nameFilters: ["PDF Documents (*.pdf)"]
    }
}