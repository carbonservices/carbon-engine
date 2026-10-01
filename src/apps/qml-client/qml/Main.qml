import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import CarbonEngineClient

ApplicationWindow {
    id: appWindow
    width: 1360
    height: 840
    minimumWidth: 1080
    minimumHeight: 680
    visible: true
    title: "Carbon ESG - AI Workspace"

    property bool isDarkMode: true
    property bool sidebarExpanded: true
    property string activeReportTitle: "TATA Steel BRSR 2026"
    property string activeReportContent: ""
    property int currentJobId: -1

    Material.theme: isDarkMode ? Material.Dark : Material.Light
    Material.accent: Material.Teal
    color: isDarkMode ? "#121417" : "#f4f6f8"

    readonly property color bgCard: isDarkMode ? "#1a1d21" : "#ffffff"
    readonly property color bgSubtle: isDarkMode ? "#22262b" : "#e9ecef"
    readonly property color borderCol: isDarkMode ? "#2d3239" : "#ced4da"
    readonly property color textMain: isDarkMode ? "#f1f3f5" : "#212529"
    readonly property color textMuted: isDarkMode ? "#8a94a0" : "#6c757d"
    readonly property color brandAccent: "#1baf7a"

    ListModel { id: statusLogModel; ListElement { entry: "System ready. Local engine standing by." } }
    ListModel { id: documentQueueModel }
    ListModel { 
        id: reportsHistoryModel
        ListElement { title: "TATA Steel BRSR 2026"; date: "2026-09-29" }
        ListElement { title: "JSW Steel Sustainability 2025"; date: "2026-08-14" }
    }

    function appendStatus(msg) {
        statusLogModel.append({ "entry": msg })
        rightPanel.scrollToBottom()
    }

    UploadClient {
        id: uploadClient
        onJobQueued: function(jobId) {
            currentJobId = jobId
            appendStatus("Job " + jobId + " queued. File submitted.")
            poller.startPolling(jobId)
        }
        onUploadFailed: function(reason) { appendStatus("Upload failed: " + reason) }
    }

    PipelineStatusPoller {
        id: poller
        onCompleted: {
            appendStatus("Extraction complete. Generating BRSR report...")
            reportClient.fetchReport(currentJobId)
        }
        onFailed: function(reason) { appendStatus("Pipeline error: " + reason) }
    }

    ReportClient {
        id: reportClient
        onReportReady: function(report) {
            appWindow.activeReportContent = report
            appendStatus("Report generated successfully.")
        }
        onReportChunkReceived: function(chunk) {
            appWindow.activeReportContent += chunk
        }
        onReportFailed: function(reason) { appendStatus("Report fetch failed: " + reason) }
    }

    FileDialog {
        id: addDocDialog
        title: "Select Document for Ingestion"
        nameFilters: ["PDF Documents (*.pdf)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            var name = path.substring(path.lastIndexOf('/') + 1)
            documentQueueModel.append({ "fileName": name })
            appendStatus("Added document: " + name)
            appWindow.activeReportContent = ""
            uploadClient.uploadDocument(selectedFile)
        }
    }

    header: ToolBar {
        background: Rectangle { color: appWindow.bgCard; border.color: appWindow.borderCol; border.width: 1 }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 12

            Image {
                source: "qrc:/logo.png" // Ensure logo.png is registered in your QRC or relative path
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                fillMode: Image.PreserveAspectFit
                mipmap: true
            }
            Label { text: "Carbon ESG - AI Workspace"; font.pixelSize: 16; font.bold: true; color: appWindow.textMain }
            Item { Layout.fillWidth: true }
            Label { text: isDarkMode ? "Dark" : "Light"; color: appWindow.textMuted; font.pixelSize: 12 }
            Switch { checked: appWindow.isDarkMode; onCheckedChanged: appWindow.isDarkMode = checked }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        LeftSidebar {
            id: leftSidebar
        }

        CentralWorkspace {
            id: centralWorkspace
        }

        RightPanel {
            id: rightPanel
        }
    }
}