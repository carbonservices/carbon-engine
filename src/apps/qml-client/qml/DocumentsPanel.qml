import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CarbonEngineClient
import "BRSRMetadata.js" as BrsrMeta

Rectangle {
    id: root
    Layout.fillWidth: true
    Layout.preferredHeight: 260
    color: Theme.bgSidebar
    radius: 4
    border.color: Theme.borderCol
    border.width: 1
    clip: true

    property var activeModel: (typeof activeDocumentsModel !== "undefined") ? activeDocumentsModel : null
    property var queueModel: (typeof documentQueueModel !== "undefined") ? documentQueueModel : null
    property int currentJobId: (typeof appWindow !== "undefined") ? appWindow.currentJobId : -1
    property bool batchConfirmed: (typeof appWindow !== "undefined") ? appWindow.batchConfirmed : false
    property bool daemonConnected: (typeof appWindow !== "undefined" && appWindow.systemPoller) ? appWindow.systemPoller.isConnected : true

    readonly property int queueCount: (root.queueModel ? root.queueModel.count : 0)
    readonly property int activeCount: (root.activeModel ? root.activeModel.count : 0)
    readonly property int totalDocCount: queueCount + activeCount

    signal addDocumentRequested()
    signal startProcessingRequested()
    signal cancelActiveJobRequested()

    // Backward-compatibility no-op helpers
    function showQueueTab() {}
    function showAttachedTab() {}

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Label {
                text: "DOCUMENTS"
                font.bold: true
                font.pixelSize: 11
                color: Theme.textMuted
            }

            Rectangle {
                visible: root.totalDocCount > 0
                radius: 8
                color: Theme.bgSubtle
                border.color: Theme.borderCol
                border.width: 1
                Layout.preferredHeight: 18
                Layout.preferredWidth: docBadgeText.implicitWidth + 10

                Text {
                    id: docBadgeText
                    anchors.centerIn: parent
                    text: root.totalDocCount
                    font.pixelSize: 10
                    font.bold: true
                    color: Theme.textBright
                }
            }

            Item { Layout.fillWidth: true }

            Label {
                visible: root.currentJobId >= 0
                text: "Report #" + root.currentJobId
                font.pixelSize: 10
                font.bold: true
                color: Theme.brandAccent
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderCol }

        // Documents List / Empty Placeholder
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ScrollView {
                id: docScrollView
                anchors.fill: parent
                visible: root.totalDocCount > 0
                contentWidth: availableWidth
                clip: true

                ColumnLayout {
                    width: docScrollView.availableWidth
                    spacing: 4

                    // 1. Queued Documents waiting for ingestion
                    Repeater {
                        model: root.queueModel
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            color: Theme.bgSubtle
                            border.color: Theme.borderSubtle
                            border.width: 1
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
                                        color: Theme.textBright
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: "Pending ingestion"
                                        font.pixelSize: 10
                                        color: Theme.textMuted
                                    }
                                }

                                Rectangle {
                                    Layout.preferredHeight: 18
                                    Layout.preferredWidth: queueBadgeText.implicitWidth + 8
                                    radius: 3
                                    color: Qt.rgba(0.23, 0.51, 0.96, 0.15)

                                    Label {
                                        id: queueBadgeText
                                        anchors.centerIn: parent
                                        text: "READY"
                                        font.pixelSize: 9
                                        font.bold: true
                                        color: Theme.brandAccent
                                    }
                                }

                                Rectangle {
                                    visible: !root.batchConfirmed
                                    Layout.preferredWidth: 20
                                    Layout.preferredHeight: 20
                                    radius: 3
                                    color: removeMouse.containsMouse ? Theme.bgHover : "transparent"

                                    Text {
                                        anchors.centerIn: parent
                                        text: "✕"
                                        color: removeMouse.containsMouse ? Theme.dangerColor : Theme.textMuted
                                        font.pixelSize: 11
                                    }

                                    MouseArea {
                                        id: removeMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            if (root.queueModel) root.queueModel.remove(index)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // 2. Active Ingested Documents
                    Repeater {
                        model: root.activeModel
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            color: Theme.bgSubtle
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
                                        color: Theme.textBright
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: {
                                            if (model.chunkCount > 0) {
                                                return model.chunkCount + " chunks" + (model.pageCount > 0 ? (" • " + model.pageCount + " pgs") : "")
                                            }
                                            if (model.status === BrsrMeta.DocumentStatus.Processing) return "Processing document..."
                                            if (model.status === BrsrMeta.DocumentStatus.Queued) return "Queued for ingestion"
                                            if (model.status === BrsrMeta.DocumentStatus.Cancelled) return "Cancelled"
                                            if (model.status === BrsrMeta.DocumentStatus.Failed) return "Failed"
                                            return "Ingested"
                                        }
                                        font.pixelSize: 10
                                        color: Theme.textMuted
                                    }
                                }

                                Rectangle {
                                    Layout.preferredHeight: 18
                                    Layout.preferredWidth: badgeText.implicitWidth + 8
                                    radius: 3
                                    color: {
                                        if (model.status === BrsrMeta.DocumentStatus.Ingested || model.status === "COMPLETED") return Qt.rgba(0.06, 0.73, 0.51, 0.15)
                                        if (model.status === BrsrMeta.DocumentStatus.Processing) return Qt.rgba(0.23, 0.51, 0.96, 0.15)
                                        if (model.status === BrsrMeta.DocumentStatus.Failed) return Qt.rgba(0.94, 0.27, 0.27, 0.15)
                                        return Qt.rgba(0.92, 0.70, 0.03, 0.15)
                                    }

                                    Label {
                                        id: badgeText
                                        anchors.centerIn: parent
                                        text: model.status || "INGESTED"
                                        font.pixelSize: 9
                                        font.bold: true
                                        color: {
                                            if (model.status === BrsrMeta.DocumentStatus.Ingested || model.status === "COMPLETED") return Theme.successColor
                                            if (model.status === BrsrMeta.DocumentStatus.Processing) return Theme.brandAccent
                                            if (model.status === BrsrMeta.DocumentStatus.Failed) return Theme.dangerColor
                                            return Theme.warningColor
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Clean Empty State
            ColumnLayout {
                anchors.centerIn: parent
                visible: root.totalDocCount === 0
                spacing: 6

                Label {
                    text: "📄"
                    font.pixelSize: 22
                    color: Theme.textMuted
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: "No documents attached"
                    font.pixelSize: 11
                    font.bold: true
                    color: Theme.textMuted
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: "Click '+ Add Document' to select PDF files\nor select a report from history."
                    font.pixelSize: 10
                    color: Theme.textMuted
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderCol }

        // Action Buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "+ Add Document"
                enabled: !root.batchConfirmed
                Layout.preferredHeight: 28
                background: Rectangle {
                    color: !parent.enabled ? "transparent"
                        : (parent.down ? Theme.bgActive
                        : (parent.hovered ? Theme.bgHover : Theme.bgSubtle))
                    border.color: Theme.borderSubtle
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? Theme.textMain : Theme.textMuted
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: root.addDocumentRequested()
            }

            Item { Layout.fillWidth: true }

            Button {
                id: ingestBtn
                text: root.batchConfirmed ? "Cancel Ingestion" : "Start Ingestion"
                enabled: root.batchConfirmed || (root.daemonConnected && root.queueCount > 0)
                Layout.preferredHeight: 28
                background: Rectangle {
                    color: !ingestBtn.enabled ? Theme.bgSubtle
                        : (root.batchConfirmed ? (ingestBtn.hovered ? "#dc2626" : Theme.dangerColor)
                        : (ingestBtn.down ? Theme.brandAccentPressed
                        : (ingestBtn.hovered ? Theme.brandAccentHover : Theme.brandAccent)))
                    radius: 3
                }
                contentItem: Text {
                    text: ingestBtn.text
                    color: ingestBtn.enabled ? (root.batchConfirmed ? "#ffffff" : Theme.textBright) : Theme.textMuted
                    font.pixelSize: 11
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    if (root.batchConfirmed) {
                        root.cancelActiveJobRequested()
                    } else {
                        root.startProcessingRequested()
                    }
                }
            }
        }
    }
}
