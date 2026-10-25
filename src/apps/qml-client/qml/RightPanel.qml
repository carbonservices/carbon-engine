import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 12
    Layout.preferredWidth: 320
    Layout.fillHeight: true
    clip: true

    function scrollToBottom() { statusLog.scrollToBottom() }
    function showQueueTab() { documentsPanel.showQueueTab() }
    function showAttachedTab() { documentsPanel.showAttachedTab() }

    DocumentsPanel {
        id: documentsPanel
        activeModel: activeDocumentsModel
        queueModel: documentQueueModel
        currentJobId: appWindow.currentJobId
        batchConfirmed: appWindow.batchConfirmed
        daemonConnected: appWindow.systemPoller.isConnected
        onAddDocumentRequested: addDocDialog.open()
        onStartProcessingRequested: appWindow.startProcessing()
        onCancelActiveJobRequested: appWindow.cancelActiveJob()
    }
    SystemStatusPanel {
        id: systemStatusPanel
        poller: appWindow.systemPoller
    }
    StatusLogPanel {
        id: statusLog
        logModel: statusLogModel
    }
}
