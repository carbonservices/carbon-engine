import QtQuick
import QtQuick.Controls

Pane {
    property string reportText: ""
    property bool isDarkMode: false

    background: Rectangle {
        color: isDarkMode ? "#1f2937" : "#ffffff"
        radius: 12
        border.color: isDarkMode ? "#374151" : "#e5e7eb"
        border.width: 1
    }

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: reportLabel.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar { }

        Label {
            id: reportLabel
            width: parent.width - 24
            x: 12
            wrapMode: Text.WordWrap
            text: reportText || "Listening for generation stream..."
            color: isDarkMode ? "#d1d5db" : "#374151"
            font.pixelSize: 15
            lineHeight: 1.5
            textFormat: Text.MarkdownText 
        }
    }
}