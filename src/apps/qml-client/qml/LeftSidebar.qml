import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    Layout.fillHeight: true
    Layout.preferredWidth: appWindow.sidebarExpanded ? 240 : 48
    color: appWindow.bgCard
    radius: 8
    border.color: appWindow.borderCol
    border.width: 1
    clip: true

    Behavior on Layout.preferredWidth { NumberAnimation { duration: 180; easing.type: Easing.InOutQuad } }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: "Reports history"
                font.bold: true
                font.pixelSize: 14
                color: appWindow.textMain
                visible: appWindow.sidebarExpanded
                Layout.fillWidth: true
            }
            ToolButton {
                text: appWindow.sidebarExpanded ? "◀" : "▶"
                onClicked: appWindow.sidebarExpanded = !appWindow.sidebarExpanded
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: appWindow.borderCol; visible: appWindow.sidebarExpanded }

        ListView {
            id: historyList
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: appWindow.sidebarExpanded
            model: reportsHistoryModel
            spacing: 6
            delegate: Rectangle {
                width: historyList.width
                height: 40
                radius: 6
                color: appWindow.activeReportTitle === model.title ? appWindow.bgSubtle : "transparent"
                border.color: appWindow.activeReportTitle === model.title ? appWindow.brandAccent : "transparent"
                border.width: 1

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 8
                    text: model.title
                    elide: Text.ElideRight
                    font.pixelSize: 13
                    color: appWindow.textMain
                }
                MouseArea { anchors.fill: parent; onClicked: appWindow.activeReportTitle = model.title }
            }
        }
    }
}