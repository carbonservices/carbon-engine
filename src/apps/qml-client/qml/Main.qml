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
    title: "Carbon Engine - BRSR ESG Analytics"
    flags: Qt.Window | Qt.FramelessWindowHint

    function safeMove() {
        if (typeof appWindow.startSystemMove === "function") {
            appWindow.startSystemMove()
        }
    }

    function safeResize(edge) {
        if (typeof appWindow.startSystemResize === "function") {
            appWindow.startSystemResize(edge)
        }
    }

    property bool isDarkMode: true
    property bool sidebarExpanded: true
    property string activeReportTitle: ""
    property string activeReportContent: ""
    property int currentJobId: -1
    property bool batchConfirmed: false

    Material.theme: isDarkMode ? Material.Dark : Material.Light
    Material.accent: brandAccent

    // VS Code Dark Modern Theme Colors
    readonly property color bgBase: isDarkMode ? "#1e1e1e" : "#ffffff"
    readonly property color bgSidebar: isDarkMode ? "#252526" : "#f3f3f3"
    readonly property color bgCard: isDarkMode ? "#252526" : "#ffffff"
    readonly property color bgHeader: isDarkMode ? "#181818" : "#f8f8f8"
    readonly property color bgSubtle: isDarkMode ? "#2a2d2e" : "#e8e8e8"
    readonly property color bgHover: isDarkMode ? "#313131" : "#e0e0e0"
    readonly property color bgActive: isDarkMode ? "#37373d" : "#d0d0d0"
    readonly property color bgInput: isDarkMode ? "#3c3c3c" : "#ffffff"
    readonly property color borderCol: isDarkMode ? "#2d2d2d" : "#e0e0e0"
    readonly property color borderSubtle: isDarkMode ? "#3c3c3c" : "#cccccc"
    readonly property color textMain: isDarkMode ? "#cccccc" : "#24292f"
    readonly property color textBright: isDarkMode ? "#ffffff" : "#000000"
    readonly property color textMuted: isDarkMode ? "#858585" : "#6e7781"
    readonly property color brandAccent: isDarkMode ? "#007acc" : "#005fb8" // VS Code Blue
    readonly property color brandAccentHover: isDarkMode ? "#1177bb" : "#004990"
    readonly property color brandAccentPressed: isDarkMode ? "#0d5c91" : "#003b75"
    readonly property color dangerColor: isDarkMode ? "#f14c4c" : "#cf222e"
    readonly property color successColor: isDarkMode ? "#89d185" : "#1a7f37"
    readonly property color warningColor: isDarkMode ? "#cca700" : "#9a6700"

    // Extraction Progress Tracking
    property bool isExtracting: false
    property int extractionStep: 0
    property int extractionTotal: 14
    property string extractionCurrentGroup: ""
    property double extractionProgressPct: 0.0
    property var missingMetricsSummary: []

    background: Rectangle {
        color: bgBase
        border.color: appWindow.borderCol
        border.width: appWindow.visibility === Window.Maximized ? 0 : 1
    }

    ListModel { id: statusLogModel; ListElement { entry: "System ready. Local engine standing by." } }
    ListModel { id: documentQueueModel }
    ListModel { id: activeDocumentsModel }
    ListModel { id: reportsHistoryModel }

    Component.onCompleted: {
        reportClient.fetchReports()
    }

    function appendStatus(msg) {
        statusLogModel.append({ "entry": msg })
        rightPanel.scrollToBottom()
    }

    function syncActiveDocuments(jobId) {
        for (var i = 0; i < reportsHistoryModel.count; ++i) {
            var item = reportsHistoryModel.get(i)
            if (item.jobId === jobId) {
                activeDocumentsModel.clear()
                var docs = item.documents
                if (docs) {
                    for (var d = 0; d < docs.length; ++d) {
                        var docObj = docs[d]
                        var path = (typeof docObj === "string") ? docObj : (docObj.file_path || "")
                        var name = (typeof docObj === "string")
                            ? path.substring(Math.max(path.lastIndexOf('/'), path.lastIndexOf('\\')) + 1)
                            : (docObj.file_name || path.substring(Math.max(path.lastIndexOf('/'), path.lastIndexOf('\\')) + 1))
                        var chunks = (typeof docObj === "object" && docObj.chunk_count !== undefined) ? docObj.chunk_count : 0
                        var pages = (typeof docObj === "object" && docObj.page_count !== undefined) ? docObj.page_count : 0
                        var status = item.status === "COMPLETED" ? "INGESTED" : item.status

                        activeDocumentsModel.append({
                            "fileName": name,
                            "filePath": path,
                            "chunkCount": chunks,
                            "pageCount": pages,
                            "status": status
                        })
                    }
                }
                break
            }
        }
    }

    function selectReport(jobId) {
        currentJobId = jobId
        for (var i = 0; i < reportsHistoryModel.count; ++i) {
            var item = reportsHistoryModel.get(i)
            if (item.jobId === jobId) {
                activeReportTitle = item.title
                syncActiveDocuments(jobId)
                if (item.extractionStatus === "ACCEPTED" || item.status === "COMPLETED" || item.extractionStatus === "READY_FOR_REVIEW") {
                    reportClient.fetchReport(jobId)
                } else {
                    activeReportContent = ""
                }
                break
            }
        }
    }

    function openReviewForCurrentJob() {
        if (currentJobId > 0) {
            reviewScreen.openForJob(currentJobId)
        }
    }

    function startProcessing() {
        if (documentQueueModel.count === 0) return
        var paths = []
        activeDocumentsModel.clear()
        for (var i = 0; i < documentQueueModel.count; ++i) {
            var item = documentQueueModel.get(i)
            paths.push(item.filePath)
            activeDocumentsModel.append({
                "fileName": item.fileName,
                "filePath": item.filePath,
                "chunkCount": 0,
                "pageCount": 0,
                "status": "QUEUED"
            })
        }
        rightPanel.showAttachedTab()
        uploadClient.uploadDocuments(paths)
    }

    UploadClient {
        id: uploadClient
        onJobQueued: function(jobId) {
            currentJobId = jobId
            batchConfirmed = true
            appendStatus("Job #" + jobId + " queued with " + activeDocumentsModel.count + " document(s).")
            for (var i = 0; i < activeDocumentsModel.count; ++i) {
                activeDocumentsModel.setProperty(i, "status", "PROCESSING")
            }
            documentQueueModel.clear()
            poller.startPolling(jobId)
            reportClient.fetchReports()
        }
        onUploadFailed: function(reason) {
            appendStatus("Upload failed: " + reason)
            batchConfirmed = false
        }
    }

    PipelineStatusPoller {
        id: poller
        onStatusUpdated: function(status, docs) {
            if (docs && docs.length > 0 && activeDocumentsModel.count === docs.length) {
                for (var i = 0; i < docs.length; ++i) {
                    var d = docs[i]
                    if (d.chunk_count !== undefined) {
                        activeDocumentsModel.setProperty(i, "chunkCount", d.chunk_count)
                    }
                    if (d.page_count !== undefined) {
                        activeDocumentsModel.setProperty(i, "pageCount", d.page_count)
                    }
                }
            }
        }
        onCompleted: {
            appendStatus("Ingestion complete. Starting per-section extraction...")
            batchConfirmed = false
            for (var i = 0; i < activeDocumentsModel.count; ++i) {
                activeDocumentsModel.setProperty(i, "status", "INGESTED")
            }
            reportClient.fetchReports()
            extractionClient.startExtraction(currentJobId)
        }
        onFailed: function(reason) {
            appendStatus("Pipeline error: " + reason)
            batchConfirmed = false
            for (var i = 0; i < activeDocumentsModel.count; ++i) {
                activeDocumentsModel.setProperty(i, "status", "FAILED")
            }
        }
    }

    ExtractionClient {
        id: extractionClient
        onExtractionStarted: {
            appWindow.isExtracting = true
            appWindow.extractionStep = 0
            appWindow.extractionTotal = 14
            appWindow.extractionCurrentGroup = "Initializing retrieval..."
            appWindow.extractionProgressPct = 0.0
            appWindow.missingMetricsSummary = []
            appendStatus("Extraction initiated across 14 BRSR statutory sections...")
            extractionPoller.startPolling(currentJobId)
        }
        onRequestFailed: function(reason) {
            appWindow.isExtracting = false
            appendStatus("Extraction request failed: " + reason)
        }
    }

    ExtractionStatusPoller {
        id: extractionPoller
        onProgressUpdated: function(step, total, group, pct) {
            appWindow.isExtracting = true
            appWindow.extractionStep = step
            appWindow.extractionTotal = total
            appWindow.extractionCurrentGroup = group
            appWindow.extractionProgressPct = pct
            if (group && group !== "") {
                appendStatus("Extracting " + group + " (" + step + "/" + total + " completed)...")
            }
        }
        onMissingMetricsReceived: function(groupLabel, missingKeys) {
            var updated = []
            for (var i = 0; i < missingMetricsSummary.length; ++i) {
                updated.push(missingMetricsSummary[i])
            }
            updated.push({ "group": groupLabel, "missing": missingKeys })
            appWindow.missingMetricsSummary = updated
            appendStatus("[Notice] " + groupLabel + ": Missing from context (" + missingKeys.join(", ") + ") - manual entry needed")
        }
        onCompleted: {
            appWindow.isExtracting = false
            appendStatus("Extraction complete (" + extractionTotal + "/" + extractionTotal + " sections processed). Opening review screen...")
            reportClient.fetchReports()
            reviewScreen.openForJob(currentJobId)
        }
        onFailed: function(reason) {
            appWindow.isExtracting = false
            appendStatus("Extraction failed: " + reason)
        }
    }

    ReportReviewScreen {
        id: reviewScreen
        onReportAccepted: function(jobId) {
            appendStatus("Report accepted. Generating final HTML...")
            reportClient.fetchReport(jobId)
            reportClient.fetchReports()
        }
    }

    ReportClient {
        id: reportClient
        onReportReady: function(report) {
            appWindow.activeReportContent = report
            appendStatus("Report generated successfully.")
        }
        onReportFailed: function(reason) { appendStatus("Report fetch failed: " + reason) }
        onReportsReceived: function(reports) {
            reportsHistoryModel.clear()
            for (var i = 0; i < reports.length; ++i) {
                var r = reports[i]
                var docList = r.documents || []
                reportsHistoryModel.append({
                    "jobId": r.job_id,
                    "title": r.title,
                    "status": r.status,
                    "extractionStatus": r.extraction_status,
                    "date": r.date,
                    "docCount": docList.length,
                    "documents": docList
                })
            }
            appendStatus("Loaded " + reports.length + " report(s) from database.")
            if (reports.length > 0 && currentJobId === -1) {
                selectReport(reports[0].job_id)
            } else if (currentJobId !== -1) {
                syncActiveDocuments(currentJobId)
            }
        }
        onDocumentsReceived: function(jobId, docs) {
            if (currentJobId === jobId) {
                activeDocumentsModel.clear()
                for (var i = 0; i < docs.length; ++i) {
                    var d = docs[i]
                    activeDocumentsModel.append({
                        "fileName": d.file_name || "",
                        "filePath": d.file_path || "",
                        "chunkCount": d.chunk_count || 0,
                        "pageCount": d.page_count || 0,
                        "status": "INGESTED"
                    })
                }
            }
        }
        onReportDeleted: function(jobId) {
            for (var i = 0; i < reportsHistoryModel.count; ++i) {
                if (reportsHistoryModel.get(i).jobId === jobId) {
                    reportsHistoryModel.remove(i)
                    break
                }
            }
            if (currentJobId === jobId) {
                currentJobId = -1
                activeReportTitle = ""
                activeReportContent = ""
                activeDocumentsModel.clear()
            }
            appendStatus("Deleted report #" + jobId + " from history.")
        }
        onReportDeleteFailed: function(reason) {
            appendStatus("Failed to delete report: " + reason)
        }
    }

    FileDialog {
        id: addDocDialog
        title: "Select Documents for Ingestion"
        nameFilters: ["PDF Documents (*.pdf)"]
        fileMode: FileDialog.OpenFiles
        onAccepted: {
            var files = []
            if (selectedFiles && selectedFiles.length > 0) {
                files = selectedFiles
            } else if (selectedFile) {
                files = [selectedFile]
            }
            for (var i = 0; i < files.length; ++i) {
                var raw = files[i].toString()
                var decoded = decodeURIComponent(raw)
                var cleanPath = decoded
                if (cleanPath.indexOf("file:///") === 0) {
                    cleanPath = cleanPath.substring(cleanPath.charAt(9) === ':' ? 8 : 7)
                } else if (cleanPath.indexOf("file://") === 0) {
                    cleanPath = cleanPath.substring(7)
                }
                var fileName = cleanPath.substring(Math.max(cleanPath.lastIndexOf('/'), cleanPath.lastIndexOf('\\')) + 1)
                if (!fileName) fileName = "Document " + (documentQueueModel.count + 1)

                var exists = false
                for (var q = 0; q < documentQueueModel.count; ++q) {
                    if (documentQueueModel.get(q).filePath === cleanPath) {
                        exists = true
                        break
                    }
                }
                if (!exists) {
                    documentQueueModel.append({
                        "fileName": fileName,
                        "filePath": cleanPath,
                        "rawUrl": raw
                    })
                    appendStatus("Added document: " + fileName)
                }
            }
            rightPanel.showQueueTab()
        }
    }

    header: Rectangle {
        id: customTitleBar
        height: 36
        color: appWindow.bgHeader
        border.color: appWindow.borderCol
        border.width: 1

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onPressed: appWindow.safeMove()
            onDoubleClicked: {
                if (appWindow.visibility === Window.Maximized) {
                    appWindow.showNormal()
                } else {
                    appWindow.showMaximized()
                }
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 0
            spacing: 10

            // Logo & Brand
            RowLayout {
                spacing: 8
                Layout.alignment: Qt.AlignVCenter

                Image {
                    id: appLogo
                    source: "logo.png"
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    fillMode: Image.PreserveAspectFit
                    mipmap: true
                    smooth: true
                }

                Label {
                    text: "Carbon Engine"
                    font.pixelSize: 12
                    font.bold: true
                    color: appWindow.textBright
                }

                Label {
                    text: "•"
                    font.pixelSize: 10
                    color: appWindow.borderSubtle
                }

                Label {
                    text: "BRSR ESG Analytics"
                    font.pixelSize: 11
                    color: appWindow.textMuted
                }
            }

            Item { Layout.fillWidth: true }

            // Centered Workspace / Report Status
            Label {
                text: appWindow.isExtracting
                    ? ("Extracting: " + (appWindow.extractionCurrentGroup !== "" ? appWindow.extractionCurrentGroup : "BRSR Sections"))
                    : (appWindow.activeReportTitle !== "" ? appWindow.activeReportTitle : "Workspace")
                font.pixelSize: 11
                color: appWindow.textMuted
                elide: Text.ElideMiddle
                Layout.maximumWidth: 450
            }

            Item { Layout.fillWidth: true }

            // Theme Toggle
            Button {
                text: appWindow.isDarkMode ? "🌙 Dark" : "☀️ Light"
                flat: true
                Layout.preferredHeight: 26
                background: Rectangle {
                    color: parent.hovered ? appWindow.bgHover : "transparent"
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: appWindow.textMain
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: appWindow.isDarkMode = !appWindow.isDarkMode
            }

            // Window Controls (Minimize, Maximize/Restore, Close)
            RowLayout {
                spacing: 0
                Layout.preferredHeight: 36

                // Minimize
                Rectangle {
                    Layout.preferredWidth: 46
                    Layout.preferredHeight: 36
                    color: minMouse.containsMouse ? appWindow.bgHover : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "—"
                        font.pixelSize: 11
                        color: appWindow.textMain
                    }

                    MouseArea {
                        id: minMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: appWindow.showMinimized()
                    }
                }

                // Maximize / Restore
                Rectangle {
                    Layout.preferredWidth: 46
                    Layout.preferredHeight: 36
                    color: maxMouse.containsMouse ? appWindow.bgHover : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: appWindow.visibility === Window.Maximized ? "🗗" : "🗖"
                        font.pixelSize: 12
                        color: appWindow.textMain
                    }

                    MouseArea {
                        id: maxMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (appWindow.visibility === Window.Maximized) {
                                appWindow.showNormal()
                            } else {
                                appWindow.showMaximized()
                            }
                        }
                    }
                }

                // Close
                Rectangle {
                    Layout.preferredWidth: 46
                    Layout.preferredHeight: 36
                    color: closeMouse.containsMouse ? "#e81123" : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        font.pixelSize: 11
                        color: closeMouse.containsMouse ? "#ffffff" : appWindow.textMain
                    }

                    MouseArea {
                        id: closeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: appWindow.close()
                    }
                }
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

    // Frameless Window Edge & Corner Resizers
    Item {
        id: windowResizers
        anchors.fill: parent
        visible: appWindow.visibility !== Window.Maximized
        z: 9999

        // Top edge
        MouseArea {
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
            anchors.leftMargin: 8; anchors.rightMargin: 8; height: 5
            cursorShape: Qt.SizeVerCursor
            onPressed: appWindow.safeResize(Qt.TopEdge)
        }
        // Bottom edge
        MouseArea {
            anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right
            anchors.leftMargin: 8; anchors.rightMargin: 8; height: 5
            cursorShape: Qt.SizeVerCursor
            onPressed: appWindow.safeResize(Qt.BottomEdge)
        }
        // Left edge
        MouseArea {
            anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
            anchors.topMargin: 8; anchors.bottomMargin: 8; width: 5
            cursorShape: Qt.SizeHorCursor
            onPressed: appWindow.safeResize(Qt.LeftEdge)
        }
        // Right edge
        MouseArea {
            anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
            anchors.topMargin: 8; anchors.bottomMargin: 8; width: 5
            cursorShape: Qt.SizeHorCursor
            onPressed: appWindow.safeResize(Qt.RightEdge)
        }
        // Top-Left corner
        MouseArea {
            anchors.top: parent.top; anchors.left: parent.left; width: 8; height: 8
            cursorShape: Qt.SizeFDiagCursor
            onPressed: appWindow.safeResize(Qt.TopEdge | Qt.LeftEdge)
        }
        // Top-Right corner
        MouseArea {
            anchors.top: parent.top; anchors.right: parent.right; width: 8; height: 8
            cursorShape: Qt.SizeBDiagCursor
            onPressed: appWindow.safeResize(Qt.TopEdge | Qt.RightEdge)
        }
        // Bottom-Left corner
        MouseArea {
            anchors.bottom: parent.bottom; anchors.left: parent.left; width: 8; height: 8
            cursorShape: Qt.SizeBDiagCursor
            onPressed: appWindow.safeResize(Qt.BottomEdge | Qt.LeftEdge)
        }
        // Bottom-Right corner
        MouseArea {
            anchors.bottom: parent.bottom; anchors.right: parent.right; width: 8; height: 8
            cursorShape: Qt.SizeFDiagCursor
            onPressed: appWindow.safeResize(Qt.BottomEdge | Qt.RightEdge)
        }
    }
}