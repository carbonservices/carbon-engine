import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Pane {
    id: root
    property bool isDarkMode: false
    signal uploadRequested(url fileUrl)

    background: Rectangle {
        color: isDarkMode ? "#1f2937" : "#ffffff"
        radius: 12
        border.color: isDarkMode ? "#374151" : "#e5e7eb"
        border.width: 1
    }

    RowLayout {
        anchors.fill: parent
        spacing: 16

        Button {
            text: "Browse PDF"
            onClicked: fileDialog.open()
            background: Rectangle {
                color: isDarkMode ? "#374151" : "#f3f4f6"
                radius: 8
            }
            contentItem: Text {
                text: parent.text
                color: isDarkMode ? "#e5e7eb" : "#1f2937"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.bold: true
            }
        }

        Label {
            id: pathLabel
            text: fileDialog.selectedFile.toString() === "" ? "No file selected..." : fileDialog.selectedFile.toString().replace("file:///", "")
            color: isDarkMode ? "#9ca3af" : "#6b7280"
            elide: Text.ElideMiddle
            Layout.fillWidth: true
            font.pixelSize: 14
        }

        Button {
            text: "Upload & Process"
            enabled: fileDialog.selectedFile.toString().length > 0
            onClicked: root.uploadRequested(fileDialog.selectedFile)
            background: Rectangle {
                color: parent.enabled ? "#3b82f6" : (isDarkMode ? "#374151" : "#e5e7eb")
                radius: 8
            }
            contentItem: Text {
                text: parent.text
                color: parent.enabled ? "#ffffff" : (isDarkMode ? "#6b7280" : "#9ca3af")
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.bold: true
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Select a document"
        nameFilters: ["PDF Documents (*.pdf)"]
    }
}