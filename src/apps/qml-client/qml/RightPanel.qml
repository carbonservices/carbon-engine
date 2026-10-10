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

    DocumentsPanel { id: documentsPanel }
    SystemStatusPanel { id: systemStatusPanel }
    StatusLogPanel { id: statusLog }
}