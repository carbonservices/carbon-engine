import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: rightPanelRoot
    spacing: 0
    property real panelWidth: (typeof appWindow !== "undefined" && appWindow.rightPanelWidth) ? appWindow.rightPanelWidth : 320
    Layout.preferredWidth: panelWidth
    Layout.minimumWidth: 250
    Layout.maximumWidth: 500
    Layout.fillHeight: true
    clip: true

    property real docPanelHeight: 250
    property real statusPanelHeight: 320

    function scrollToBottom() { statusLog.scrollToBottom() }

    DocumentsPanel {
        id: documentsPanel
        Layout.fillWidth: true
        Layout.preferredHeight: rightPanelRoot.docPanelHeight
        Layout.minimumHeight: 140
        Layout.maximumHeight: 400
        activeModel: activeDocumentsModel
        queueModel: documentQueueModel
        currentJobId: appWindow.currentJobId
        batchConfirmed: appWindow.batchConfirmed
        daemonConnected: appWindow.systemPoller.isConnected
        onAddDocumentRequested: addDocDialog.open()
        onStartProcessingRequested: appWindow.startProcessing()
        onCancelActiveJobRequested: appWindow.cancelActiveJob()
    }

    // Vertical Splitter between Documents and SystemStatus
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 8
        color: "transparent"

        Rectangle {
            anchors.centerIn: parent
            height: 2
            width: 36
            radius: 1
            color: split1Mouse.containsMouse || split1Mouse.pressed ? Theme.brandAccent : Theme.borderSubtle
        }

        MouseArea {
            id: split1Mouse
            anchors.fill: parent
            anchors.topMargin: -2
            anchors.bottomMargin: -2
            hoverEnabled: true
            cursorShape: Qt.SplitVCursor

            property real dragStartGlobalY: 0
            property real dragStartHeight: 0

            onPressed: function(mouse) {
                var pt = mapToItem(rightPanelRoot, 0, mouse.y)
                dragStartGlobalY = pt.y
                dragStartHeight = rightPanelRoot.docPanelHeight
            }
            onPositionChanged: function(mouse) {
                if (pressed) {
                    var pt = mapToItem(rightPanelRoot, 0, mouse.y)
                    var delta = pt.y - dragStartGlobalY
                    var newH = Math.max(140, Math.min(380, dragStartHeight + delta))
                    rightPanelRoot.docPanelHeight = newH
                }
            }
        }
    }

    SystemStatusPanel {
        id: systemStatusPanel
        Layout.fillWidth: true
        panelHeight: rightPanelRoot.statusPanelHeight
        poller: appWindow.systemPoller
    }

    // Vertical Splitter between SystemStatus and StatusLog
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 8
        color: "transparent"

        Rectangle {
            anchors.centerIn: parent
            height: 2
            width: 36
            radius: 1
            color: split2Mouse.containsMouse || split2Mouse.pressed ? Theme.brandAccent : Theme.borderSubtle
        }

        MouseArea {
            id: split2Mouse
            anchors.fill: parent
            anchors.topMargin: -2
            anchors.bottomMargin: -2
            hoverEnabled: true
            cursorShape: Qt.SplitVCursor

            property real dragStartGlobalY: 0
            property real dragStartHeight: 0

            onPressed: function(mouse) {
                var pt = mapToItem(rightPanelRoot, 0, mouse.y)
                dragStartGlobalY = pt.y
                dragStartHeight = rightPanelRoot.statusPanelHeight
            }
            onPositionChanged: function(mouse) {
                if (pressed) {
                    var pt = mapToItem(rightPanelRoot, 0, mouse.y)
                    var delta = pt.y - dragStartGlobalY
                    var newH = Math.max(180, Math.min(420, dragStartHeight + delta))
                    rightPanelRoot.statusPanelHeight = newH
                }
            }
        }
    }

    StatusLogPanel {
        id: statusLog
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 90
        logModel: statusLogModel
    }
}
