import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CarbonEngineClient
import "BRSRMetadata.js" as BrsrMeta

Rectangle {
    id: root
    Layout.fillHeight: true
    Layout.preferredWidth: root.sidebarExpanded ? 260 : 44
    color: Theme.bgSidebar
    radius: 4
    border.color: Theme.borderCol
    border.width: 1
    clip: true

    property bool sidebarExpanded: (typeof appWindow !== "undefined") ? appWindow.sidebarExpanded : true
    property var historyModel: (typeof reportsHistoryModel !== "undefined") ? reportsHistoryModel : null
    property int currentJobId: (typeof appWindow !== "undefined") ? appWindow.currentJobId : -1
    property bool isExtracting: (typeof appWindow !== "undefined") ? appWindow.isExtracting : false
    property bool batchConfirmed: (typeof appWindow !== "undefined") ? appWindow.batchConfirmed : false

    signal toggleExpanded()
    signal reportSelected(int jobId)
    signal cancelJobRequested(int jobId)
    signal deleteReportRequested(int jobId)
    signal refreshReportsRequested()

    Behavior on Layout.preferredWidth { NumberAnimation { duration: 180; easing.type: Easing.InOutQuad } }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            Layout.alignment: Qt.AlignTop

            Rectangle {
                id: arrowBtn
                width: 26; height: 26; radius: 3
                color: arrowMouse.containsMouse ? Theme.bgHover : "transparent"
                anchors.left: root.sidebarExpanded ? parent.left : undefined
                anchors.horizontalCenter: root.sidebarExpanded ? undefined : parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    anchors.centerIn: parent
                    text: root.sidebarExpanded ? "‹" : "›"
                    color: arrowMouse.containsMouse ? Theme.textBright : Theme.brandAccent
                    font.pixelSize: 16
                    font.bold: true
                }

                MouseArea {
                    id: arrowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.toggleExpanded()
                }
            }

            Label {
                text: "REPORTS HISTORY"
                font.bold: true
                font.pixelSize: 11
                color: Theme.textMuted
                visible: root.sidebarExpanded
                anchors.left: arrowBtn.right
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
            }

            Rectangle {
                id: refreshBtn
                width: 24; height: 24; radius: 3
                visible: root.sidebarExpanded
                color: refreshMouse.containsMouse ? Theme.bgHover : "transparent"
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    anchors.centerIn: parent
                    text: "↻"
                    color: refreshMouse.containsMouse ? Theme.textBright : Theme.textMuted
                    font.pixelSize: 14
                }

                MouseArea {
                    id: refreshMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.refreshReportsRequested()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.borderCol
            visible: root.sidebarExpanded
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.sidebarExpanded

            ListView {
                id: historyList
                anchors.fill: parent
                visible: root.historyModel && root.historyModel.count > 0
                model: root.historyModel
                spacing: 4
                boundsBehavior: Flickable.StopAtBounds
                clip: true

                delegate: Rectangle {
                    id: delegateItem
                    width: historyList.width
                    height: 44
                    radius: 3
                    color: (root.currentJobId === model.jobId)
                           ? Theme.bgActive
                           : (rowHover.hovered ? Theme.bgSubtle : "transparent")

                    HoverHandler {
                        id: rowHover
                    }

                    Rectangle {
                        width: 2
                        height: parent.height
                        color: Theme.brandAccent
                        anchors.left: parent.left
                        visible: root.currentJobId === model.jobId
                    }

                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: actionsRow.left
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: (root.currentJobId === model.jobId) ? 10 : 8
                        anchors.rightMargin: 4
                        spacing: 2

                        Label {
                            Layout.fillWidth: true
                            text: model.title
                            elide: Text.ElideRight
                            font.pixelSize: 12
                            color: (root.currentJobId === model.jobId) ? Theme.textBright : Theme.textMain
                            font.bold: root.currentJobId === model.jobId
                        }

                        RowLayout {
                            spacing: 6
                            Label {
                                text: (model.docCount > 0 ? (model.docCount + " doc" + (model.docCount > 1 ? "s" : "") + " • ") : "") + (model.date || "")
                                font.pixelSize: 10
                                color: Theme.textMuted
                            }
                            Label {
                                text: (model.extractionStatus === "CANCELLED" || model.status === "CANCELLED") ? "CANCELLED" : (model.status || "")
                                font.pixelSize: 9
                                font.bold: true
                                color: {
                                    if (model.status === BrsrMeta.JobStatus.Completed) return Theme.successColor
                                    if (model.status === "CANCELLED" || model.extractionStatus === "CANCELLED") return Theme.warningColor
                                    if (model.status === "FAILED" || model.extractionStatus === "FAILED") return Theme.dangerColor
                                    return Theme.brandAccent
                                }
                            }
                        }
                    }

                    RowLayout {
                        id: actionsRow
                        z: 2
                        anchors.right: parent.right
                        anchors.rightMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2
                        visible: rowHover.hovered

                        // Delete button
                        Rectangle {
                            id: deleteBtn
                            width: 22
                            height: 22
                            radius: 3
                            color: deleteMouse.containsMouse ? Theme.bgHover : "transparent"
                            visible: rowHover.hovered

                            Text {
                                anchors.centerIn: parent
                                text: "✕"
                                color: deleteMouse.containsMouse ? Theme.dangerColor : Theme.textMuted
                                font.pixelSize: 11
                            }

                            ToolTip.visible: deleteMouse.containsMouse
                            ToolTip.delay: 300
                            ToolTip.text: "Delete report from database"

                            MouseArea {
                                id: deleteMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.deleteReportRequested(model.jobId)
                                }
                            }
                        }
                    }

                    MouseArea {
                        id: itemMouse
                        anchors.left: parent.left
                        anchors.right: actionsRow.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.reportSelected(model.jobId)
                        }
                    }
                }
            }

            ColumnLayout {
                anchors.centerIn: parent
                visible: !root.historyModel || root.historyModel.count === 0
                spacing: 6

                Label {
                    text: "No reports yet"
                    font.pixelSize: 12
                    font.bold: true
                    color: Theme.textMuted
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: "Add documents to start"
                    font.pixelSize: 11
                    color: Theme.textMuted
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !root.sidebarExpanded
        }
    }
}