import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    Layout.fillHeight: true
    Layout.preferredWidth: appWindow.sidebarExpanded ? 260 : 44
    color: appWindow.bgSidebar
    radius: 4
    border.color: appWindow.borderCol
    border.width: 1
    clip: true

    Behavior on Layout.preferredWidth { NumberAnimation { duration: 180; easing.type: Easing.InOutQuad } }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 28

            // Toggle button fixed on the LEFT - position does NOT change on collapse
            Rectangle {
                id: arrowBtn
                width: 26; height: 26; radius: 3
                color: arrowMouse.containsMouse ? appWindow.bgHover : "transparent"
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    anchors.centerIn: parent
                    text: appWindow.sidebarExpanded ? "‹" : "›"
                    color: arrowMouse.containsMouse ? appWindow.textBright : appWindow.brandAccent
                    font.pixelSize: 16
                    font.bold: true
                }

                MouseArea {
                    id: arrowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: appWindow.sidebarExpanded = !appWindow.sidebarExpanded
                }
            }

            Label {
                text: "REPORTS HISTORY"
                font.bold: true
                font.pixelSize: 11
                color: appWindow.textMuted
                visible: appWindow.sidebarExpanded
                anchors.left: arrowBtn.right
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
            }

            Rectangle {
                id: refreshBtn
                width: 24; height: 24; radius: 3
                visible: appWindow.sidebarExpanded
                color: refreshMouse.containsMouse ? appWindow.bgHover : "transparent"
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    anchors.centerIn: parent
                    text: "↻"
                    color: refreshMouse.containsMouse ? appWindow.textBright : appWindow.textMuted
                    font.pixelSize: 14
                }

                MouseArea {
                    id: refreshMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: reportClient.fetchReports()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: appWindow.borderCol
            visible: appWindow.sidebarExpanded
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: appWindow.sidebarExpanded

            ListView {
                id: historyList
                anchors.fill: parent
                visible: reportsHistoryModel.count > 0
                model: reportsHistoryModel
                spacing: 4
                boundsBehavior: Flickable.StopAtBounds
                clip: true

                delegate: Rectangle {
                    id: delegateItem
                    width: historyList.width
                    height: 44
                    radius: 3
                    color: (appWindow.currentJobId === model.jobId)
                           ? appWindow.bgActive
                           : (rowHover.hovered ? appWindow.bgSubtle : "transparent")

                    HoverHandler {
                        id: rowHover
                    }

                    // Selection accent bar
                    Rectangle {
                        width: 2
                        height: parent.height
                        color: appWindow.brandAccent
                        anchors.left: parent.left
                        visible: appWindow.currentJobId === model.jobId
                    }

                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: deleteBtn.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: (appWindow.currentJobId === model.jobId) ? 10 : 8
                        anchors.rightMargin: 4
                        spacing: 2

                        Label {
                            Layout.fillWidth: true
                            text: model.title
                            elide: Text.ElideRight
                            font.pixelSize: 12
                            color: (appWindow.currentJobId === model.jobId) ? appWindow.textBright : appWindow.textMain
                            font.bold: appWindow.currentJobId === model.jobId
                        }

                        RowLayout {
                            spacing: 6
                            Label {
                                text: (model.docCount > 0 ? (model.docCount + " doc" + (model.docCount > 1 ? "s" : "") + " • ") : "") + (model.date || "")
                                font.pixelSize: 10
                                color: appWindow.textMuted
                            }
                            Label {
                                text: model.status || ""
                                font.pixelSize: 9
                                font.bold: true
                                color: model.status === "COMPLETED" ? appWindow.successColor : appWindow.textMuted
                            }
                        }
                    }

                    // Delete button (visible on hover)
                    Rectangle {
                        id: deleteBtn
                        z: 2
                        width: 22
                        height: 22
                        radius: 3
                        anchors.right: parent.right
                        anchors.rightMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        color: deleteMouse.containsMouse ? appWindow.bgHover : "transparent"
                        visible: rowHover.hovered

                        Text {
                            anchors.centerIn: parent
                            text: "✕"
                            color: deleteMouse.containsMouse ? appWindow.dangerColor : appWindow.textMuted
                            font.pixelSize: 11
                        }

                        MouseArea {
                            id: deleteMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                reportClient.deleteReport(model.jobId)
                            }
                        }
                    }

                    MouseArea {
                        id: itemMouse
                        anchors.left: parent.left
                        anchors.right: deleteBtn.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            appWindow.selectReport(model.jobId)
                            rightPanel.showAttachedTab()
                        }
                    }
                }
            }

            // Empty state when no reports exist in database
            ColumnLayout {
                anchors.centerIn: parent
                visible: reportsHistoryModel.count === 0
                spacing: 6

                Label {
                    text: "No reports yet"
                    font.pixelSize: 12
                    font.bold: true
                    color: appWindow.textMuted
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: "Add documents to start"
                    font.pixelSize: 11
                    color: appWindow.textMuted
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }
    }
}