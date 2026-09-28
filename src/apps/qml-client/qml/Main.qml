import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import CarbonEngineClient

ApplicationWindow {
    id: window
    width: 1000
    height: 750
    visible: true
    title: "Carbon ESG Engine"

    property bool isDarkMode: true
    property int currentJobId: -1

    // Apply Material Styling dynamically
    Material.theme: isDarkMode ? Material.Dark : Material.Light
    Material.accent: Material.Blue
    color: isDarkMode ? "#111827" : "#f9fafb"

    header: ToolBar {
        background: Rectangle {
            color: isDarkMode ? "#1f2937" : "#ffffff"
            border.color: isDarkMode ? "#374151" : "#e5e7eb"
            border.width: 1
        }
        RowLayout {
            anchors.fill: parent
            anchors.margins: 16
            Label {
                text: "Carbon Engine Client"
                font.pixelSize: 18
                font.bold: true
                color: isDarkMode ? "#f3f4f6" : "#1f2937"
                Layout.fillWidth: true
            }
            Switch {
                text: isDarkMode ? "Dark Mode" : "Light Mode"
                checked: isDarkMode
                onCheckedChanged: isDarkMode = checked
            }
        }
    }

    UploadClient {
        id: uploadClient
        onJobQueued: function(jobId) {
            currentJobId = jobId
            statusDetailLabel.text = "Job " + jobId + " queued. Waiting for local worker..."
            pipeline.visible = true
            reportView.visible = false
            poller.startPolling(jobId)
        }
        onUploadFailed: function(reason) {
            statusDetailLabel.text = "Upload failed: " + reason
        }
    }

    PipelineStatusPoller {
        id: poller
        onCompleted: {
            statusDetailLabel.text = "Extraction complete. Qwen is evaluating the metrics..."
            reportClient.fetchReport(currentJobId)
        }
        onFailed: function(reason) {
            statusDetailLabel.text = "Pipeline failed: " + reason
        }
    }

    ReportClient {
        id: reportClient
        onReportReady: function(report) {
            reportView.reportText = report
            reportView.visible = true
            statusDetailLabel.text = "Report generated successfully."
        }
        onReportChunkReceived: function(chunk) {
            reportView.visible = true
            reportView.reportText += chunk
        }
        onReportFailed: function(reason) {
            statusDetailLabel.text = "Report fetch failed: " + reason
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 24

        FilePickerBar {
            Layout.fillWidth: true
            isDarkMode: window.isDarkMode
            onUploadRequested: function(fileUrl) {
                statusDetailLabel.text = "Transmitting document to server..."
                reportView.reportText = ""
                pipeline.visible = true
                reportView.visible = false
                uploadClient.uploadDocument(fileUrl)
            }
        }

        ColumnLayout {
            id: pipeline
            visible: false
            Layout.fillWidth: true
            spacing: 16

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 0

                Repeater {
                    id: stageRepeater
                    model: poller
                    delegate: PipelineStageIndicator {
                        stageName: model.name
                        stageStatus: model.status
                        isLast: index === stageRepeater.count - 1
                        isDarkMode: window.isDarkMode
                    }
                }
            }

            Label {
                id: statusDetailLabel
                text: "Ready"
                color: isDarkMode ? "#9ca3af" : "#6b7280"
                font.italic: true
                font.pixelSize: 14
                Layout.alignment: Qt.AlignHCenter
            }
        }

        ReportView {
            id: reportView
            visible: false
            Layout.fillWidth: true
            Layout.fillHeight: true
            isDarkMode: window.isDarkMode
        }
    }
}