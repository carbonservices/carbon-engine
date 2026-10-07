import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    spacing: 0

    property string stageName: ""
    property int stageStatus: 0 
    property bool isLast: false

    readonly property color activeColor: appWindow.brandAccent 
    readonly property color doneColor: "#10b981" 
    readonly property color failedColor: "#ef4444" 
    readonly property color pendingBg: appWindow.bgSubtle
    readonly property color textColor: appWindow.textMain

    ColumnLayout {
        spacing: 8
        Layout.alignment: Qt.AlignTop

        Rectangle {
            width: 36
            height: 36
            radius: 18
            color: stageStatus === 2 ? doneColor : (stageStatus === 3 ? failedColor : pendingBg)
            border.color: stageStatus === 1 ? activeColor : "transparent"
            border.width: 2
            Layout.alignment: Qt.AlignHCenter

            BusyIndicator {
                anchors.fill: parent
                anchors.margins: 6
                running: stageStatus === 1
                visible: stageStatus === 1
            }

            Label {
                anchors.centerIn: parent
                text: stageStatus === 2 ? "✓" : (stageStatus === 3 ? "!" : "")
                color: "#0a192f" // Deep navy for high contrast against the colored circles
                font.bold: true
                font.pixelSize: 16
                visible: stageStatus === 2 || stageStatus === 3
            }
        }

        Label {
            text: stageName
            color: textColor
            font.bold: stageStatus === 1
            font.pixelSize: 14
            Layout.alignment: Qt.AlignHCenter
        }
    }

    // Connecting progress line
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredWidth: 80
        Layout.alignment: Qt.AlignTop
        Layout.topMargin: 17
        height: 2
        color: stageStatus === 2 ? doneColor : pendingBg
        visible: !isLast
    }
}