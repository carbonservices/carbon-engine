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
    Material.accent: brandAccent

    // Updated Navy Blue Theme Colors
    readonly property color bgBaseStart: isDarkMode ? "#0a192f" : "#f0f4f8"
    readonly property color bgBaseEnd: isDarkMode ? "#020c1b" : "#d9e2ec"
    readonly property color bgCard: isDarkMode ? "#112240" : "#ffffff"
    readonly property color bgSubtle: isDarkMode ? "#233554" : "#e0e6ed"
    readonly property color borderCol: isDarkMode ? "#233554" : "#ccd6f6"
    readonly property color textMain: isDarkMode ? "#ccd6f6" : "#0a192f"
    readonly property color textMuted: isDarkMode ? "#8892b0" : "#495670"
    readonly property color brandAccent: "#64ffda" // Teal accent

    background: Rectangle {
        gradient: Gradient {
            GradientStop { position: 0.0; color: bgBaseStart }
            GradientStop { position: 1.0; color: bgBaseEnd }
        }
    }

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
                source: "qrc:/logo.png"
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                fillMode: Image.PreserveAspectFit
                mipmap: true
            }
            
            // Removed redundant window title label to prevent double title bars
            Item { Layout.fillWidth: true }
            
            // Replaced large Switch with proportional themed button
            Button {
                text: appWindow.isDarkMode ? "🌙 Dark Mode" : "☀️ Light Mode"
                background: Rectangle {
                    color: appWindow.bgSubtle
                    radius: 6
                    border.color: appWindow.borderCol
                }
                contentItem: Text {
                    text: parent.text
                    color: appWindow.textMain
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: appWindow.isDarkMode = !appWindow.isDarkMode
                Layout.preferredHeight: 32
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12
        clip: true

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