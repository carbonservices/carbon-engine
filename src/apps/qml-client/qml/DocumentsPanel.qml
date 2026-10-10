import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    Layout.fillWidth: true
    Layout.preferredHeight: 260
    color: appWindow.bgSidebar
    radius: 4
    border.color: appWindow.borderCol
    border.width: 1
    clip: true

    property int tabIndex: 0

    function showQueueTab() { tabIndex = 1 }
    function showAttachedTab() { tabIndex = 0 }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        // Top Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Label {
                text: "DOCUMENTS"
                font.bold: true
                font.pixelSize: 11
                color: appWindow.textMuted
            }

            Item { Layout.fillWidth: true }

            Label {
                visible: appWindow.currentJobId >= 0
                text: "Report #" + appWindow.currentJobId
                font.pixelSize: 10
                font.bold: true
                color: appWindow.brandAccent
            }
        }

        // Segmented Tab Switcher
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                radius: 3
                color: root.tabIndex === 0 ? appWindow.bgActive : appWindow.bgSubtle
                border.color: root.tabIndex === 0 ? appWindow.brandAccent : appWindow.borderSubtle
                border.width: 1

                Label {
                    anchors.centerIn: parent
                    text: "Attached (" + activeDocumentsModel.count + ")"
                    font.pixelSize: 11
                    font.bold: root.tabIndex === 0
                    color: root.tabIndex === 0 ? appWindow.textBright : appWindow.textMuted
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.tabIndex = 0
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                radius: 3
                color: root.tabIndex === 1 ? appWindow.bgActive : appWindow.bgSubtle
                border.color: root.tabIndex === 1 ? appWindow.brandAccent : appWindow.borderSubtle
                border.width: 1

                Label {
                    anchors.centerIn: parent
                    text: documentQueueModel.count > 0 ? ("Queue (" + documentQueueModel.count + ")") : "Staging"
                    font.pixelSize: 11
                    font.bold: root.tabIndex === 1
                    color: documentQueueModel.count > 0 ? appWindow.brandAccent : (root.tabIndex === 1 ? appWindow.textBright : appWindow.textMuted)
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.tabIndex = 1
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: appWindow.borderCol }

        // Content Area
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            // TAB 0: Attached Documents to Active Report
            Item {
                anchors.fill: parent
                visible: root.tabIndex === 0

                ListView {
                    id: attachedList
                    anchors.fill: parent
                    visible: activeDocumentsModel.count > 0
                    model: activeDocumentsModel
                    spacing: 4
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        width: attachedList.width
                        height: 36
                        color: appWindow.bgSubtle
                        radius: 3

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 8

                            Text {
                                text: "📄"
                                font.pixelSize: 14
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1

                                Label {
                                    Layout.fillWidth: true
                                    text: model.fileName
                                    font.pixelSize: 11
                                    font.bold: true
                                    elide: Text.ElideMiddle
                                    color: appWindow.textBright
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: {
                                        if (model.chunkCount > 0) {
                                            return model.chunkCount + " chunks" + (model.pageCount > 0 ? (" • " + model.pageCount + " pgs") : "")
                                        }
                                        if (model.status === "PROCESSING") return "Processing document..."
                                        if (model.status === "QUEUED") return "Queued for ingestion"
                                        return "Ingested"
                                    }
                                    font.pixelSize: 10
                                    color: appWindow.textMuted
                                }
                            }

                            Rectangle {
                                Layout.preferredHeight: 18
                                Layout.preferredWidth: badgeText.implicitWidth + 8
                                radius: 3
                                color: {
                                    if (model.status === "INGESTED") return Qt.rgba(0.06, 0.73, 0.51, 0.15)
                                    if (model.status === "PROCESSING") return Qt.rgba(0.23, 0.51, 0.96, 0.15)
                                    if (model.status === "FAILED") return Qt.rgba(0.94, 0.27, 0.27, 0.15)
                                    return Qt.rgba(0.92, 0.70, 0.03, 0.15)
                                }

                                Label {
                                    id: badgeText
                                    anchors.centerIn: parent
                                    text: model.status || "INGESTED"
                                    font.pixelSize: 9
                                    font.bold: true
                                    color: {
                                        if (model.status === "INGESTED") return appWindow.successColor
                                        if (model.status === "PROCESSING") return appWindow.brandAccent
                                        if (model.status === "FAILED") return appWindow.dangerColor
                                        return "#eab308"
                                    }
                                }
                            }
                        }
                    }
                }

                // Empty state for Attached Documents
                ColumnLayout {
                    anchors.centerIn: parent
                    visible: activeDocumentsModel.count === 0
                    spacing: 4

                    Label {
                        text: "No documents attached"
                        font.pixelSize: 11
                        font.bold: true
                        color: appWindow.textMuted
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Label {
                        text: "Click '+ Add Document' to stage files"
                        font.pixelSize: 10
                        color: appWindow.textMuted
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            // TAB 1: Staging Queue
            Item {
                anchors.fill: parent
                visible: root.tabIndex === 1

                ListView {
                    id: queueList
                    anchors.fill: parent
                    visible: documentQueueModel.count > 0
                    model: documentQueueModel
                    spacing: 4
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        width: queueList.width
                        height: 30
                        color: appWindow.bgSubtle
                        radius: 3

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 8

                            Text {
                                text: "📄"
                                font.pixelSize: 12
                            }

                            Label {
                                Layout.fillWidth: true
                                text: model.fileName
                                elide: Text.ElideMiddle
                                font.pixelSize: 11
                                color: appWindow.textMain
                            }

                            Rectangle {
                                visible: !appWindow.batchConfirmed
                                Layout.preferredWidth: 18
                                Layout.preferredHeight: 18
                                radius: 3
                                color: removeMouse.containsMouse ? appWindow.bgHover : "transparent"

                                Text {
                                    anchors.centerIn: parent
                                    text: "✕"
                                    color: removeMouse.containsMouse ? appWindow.dangerColor : appWindow.textMuted
                                    font.pixelSize: 10
                                }

                                MouseArea {
                                    id: removeMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: documentQueueModel.remove(index)
                                }
                            }
                        }
                    }
                }

                // Empty state for Staging Queue
                ColumnLayout {
                    anchors.centerIn: parent
                    visible: documentQueueModel.count === 0
                    spacing: 4

                    Label {
                        text: "Staging queue is empty"
                        font.pixelSize: 11
                        font.bold: true
                        color: appWindow.textMuted
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Label {
                        text: "Click '+ Add Document' to select PDFs"
                        font.pixelSize: 10
                        color: appWindow.textMuted
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: appWindow.borderCol }

        // Bottom Action Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "+ Add Document"
                enabled: !appWindow.batchConfirmed
                Layout.preferredHeight: 28
                background: Rectangle {
                    color: !parent.enabled ? "transparent"
                        : (parent.down ? appWindow.bgActive
                        : (parent.hovered ? appWindow.bgHover : appWindow.bgSubtle))
                    border.color: appWindow.borderSubtle
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? appWindow.textMain : appWindow.textMuted
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: addDocDialog.open()
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Start Ingestion"
                enabled: documentQueueModel.count > 0 && !appWindow.batchConfirmed
                Layout.preferredHeight: 28
                background: Rectangle {
                    color: !parent.enabled ? appWindow.bgSubtle
                        : (parent.down ? appWindow.brandAccentPressed
                        : (parent.hovered ? appWindow.brandAccentHover : appWindow.brandAccent))
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? appWindow.textBright : appWindow.textMuted
                    font.pixelSize: 11
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    appWindow.startProcessing()
                }
            }
        }
    }
}
