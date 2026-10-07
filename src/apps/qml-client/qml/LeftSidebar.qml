import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    Layout.fillHeight: true
    Layout.preferredWidth: appWindow.sidebarExpanded ? 240 : 60
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

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 32

            Label {
                text: "Reports history"
                font.bold: true
                font.pixelSize: 14
                color: appWindow.textMain
                visible: appWindow.sidebarExpanded
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
            }

            Rectangle {
                width: 32; height: 32; radius: 6
                color: arrowMouse.containsMouse ? appWindow.bgSubtle : "transparent"
                border.color: arrowMouse.containsMouse ? appWindow.borderCol : "transparent"
                border.width: 1
                
                // Anchors update dynamically to keep arrow centered when collapsed
                anchors.right: appWindow.sidebarExpanded ? parent.right : undefined
                anchors.horizontalCenter: appWindow.sidebarExpanded ? undefined : parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    anchors.centerIn: parent
                    text: appWindow.sidebarExpanded ? "◀" : "▶"
                    color: appWindow.brandAccent
                    font.pixelSize: 14
                }
                
                MouseArea {
                    id: arrowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: appWindow.sidebarExpanded = !appWindow.sidebarExpanded
                }
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
            boundsBehavior: Flickable.StopAtBounds
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
                    anchors.margins: 12
                    text: model.title
                    elide: Text.ElideRight
                    font.pixelSize: 13
                    color: appWindow.activeReportTitle === model.title ? appWindow.brandAccent : appWindow.textMain
                    font.bold: appWindow.activeReportTitle === model.title
                }
                MouseArea { anchors.fill: parent; onClicked: appWindow.activeReportTitle = model.title }
            }
        }
    }
}