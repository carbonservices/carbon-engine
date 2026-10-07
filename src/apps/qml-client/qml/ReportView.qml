import QtQuick
import QtQuick.Controls

Pane {
    property string reportText: ""
    
    background: Rectangle {
        color: appWindow.bgCard
        radius: 12
        border.color: appWindow.borderCol
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
            color: appWindow.textMain
            font.pixelSize: 15
            lineHeight: 1.5
            textFormat: Text.MarkdownText 
        }
    }
}