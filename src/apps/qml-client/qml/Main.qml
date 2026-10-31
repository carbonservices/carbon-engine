import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import CarbonEngineClient
import "BRSRMetadata.js" as BrsrMeta

ApplicationWindow {
    id: appWindow
    width: 1360
    height: 840
    minimumWidth: 1080
    minimumHeight: 680
    visible: true
    title: "Carbon ESG (" + systemPoller.workloadTitle + ")"
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

    property bool sidebarExpanded: true
    property real leftSidebarWidth: 260
    property real rightPanelWidth: 320
    property string activeReportTitle: ""
    property string activeReportContent: ""
    property int currentJobId: -1
    property bool isCurrentReportLocked: false
    property bool batchConfirmed: false

    Material.theme: Theme.isDarkMode ? Material.Dark : Material.Light
    Material.accent: Theme.brandAccent

    // Extraction Progress Tracking
    property bool isExtracting: false
    property int extractionStep: 0
    property int extractionTotal: 0
    property string extractionCurrentGroup: ""
    property double extractionProgressPct: 0.0
    property var missingMetricsSummary: []

    background: Rectangle {
        color: Theme.bgBase
        border.color: Theme.borderCol
        border.width: appWindow.visibility === Window.Maximized ? 0 : 1
    }

    ListModel { id: statusLogModel; ListElement { entry: "Carbon ESG initialized. Connecting to Carbon Engine..." } }
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

    property var rawReportsMap: ({})

    function syncActiveDocuments(jobId) {
        if (jobId <= 0) return
        var report = rawReportsMap[jobId]
        if (report && report.documents && report.documents.length > 0) {
            activeDocumentsModel.clear()
            for (var d = 0; d < report.documents.length; ++d) {
                var docObj = report.documents[d]
                var path = (typeof docObj === "string") ? docObj : (docObj.file_path || "")
                var name = (typeof docObj === "string")
                    ? path.substring(Math.max(path.lastIndexOf('/'), path.lastIndexOf('\\')) + 1)
                    : (docObj.file_name || path.substring(Math.max(path.lastIndexOf('/'), path.lastIndexOf('\\')) + 1))
                var chunks = (typeof docObj === "object" && docObj.chunk_count !== undefined) ? docObj.chunk_count : 0
                var pages = (typeof docObj === "object" && docObj.page_count !== undefined) ? docObj.page_count : 0
                var status = (typeof docObj === "object" && docObj.status) ? docObj.status : (report.status === "COMPLETED" ? "INGESTED" : report.status)

                activeDocumentsModel.append({
                    "fileName": name,
                    "filePath": path,
                    "chunkCount": chunks,
                    "pageCount": pages,
                    "status": status
                })
            }
        }
        reportClient.fetchDocuments(jobId)
    }

    function selectReport(jobId) {
        currentJobId = jobId
        syncActiveDocuments(jobId)
        for (var i = 0; i < reportsHistoryModel.count; ++i) {
            var item = reportsHistoryModel.get(i)
            if (item.jobId === jobId) {
                activeReportTitle = item.title
                isCurrentReportLocked = (item.reportLocked === true || item.extractionStatus === "ACCEPTED")
                if (item.extractionStatus === "ACCEPTED" || item.status === "COMPLETED" || item.extractionStatus === "READY_FOR_REVIEW") {
                    reportClient.fetchReport(jobId)
                    extractionPoller.fetchStatusOnly(jobId)
                } else {
                    activeReportContent = ""
                }
                break
            }
        }
    }

    function openReviewForCurrentJob() {
        if (currentJobId > 0) {
            reviewScreen.openForJob(currentJobId, isCurrentReportLocked)
        }
    }

    function openBugReportModal() {
        bugReportModal.open()
    }

    function openFeedbackModal() {
        feedbackModal.open()
    }

    function cancelActiveJob() {
        if (currentJobId <= 0) return
        var targetId = currentJobId
        appendStatus("[Cancel Requested] Cancelling Job #" + targetId + "...")
        if (isExtracting) {
            extractionClient.cancelExtraction(targetId)
            extractionPoller.stopPolling()
            isExtracting = false
        }
        if (batchConfirmed) {
            uploadClient.cancelJob(targetId)
            poller.stopPolling()
            batchConfirmed = false
        }
        for (var i = 0; i < activeDocumentsModel.count; ++i) {
            activeDocumentsModel.setProperty(i, "status", "CANCELLED")
        }
        reportClient.fetchReports()
    }



    function cancelJobById(jobId) {
        if (jobId <= 0) return
        appendStatus("[Cancel Requested] Cancelling Job #" + jobId + " on server...")
        uploadClient.cancelJob(jobId)
        extractionClient.cancelExtraction(jobId)
        if (jobId === currentJobId) {
            isExtracting = false
            batchConfirmed = false
            extractionPoller.stopPolling()
            poller.stopPolling()
            for (var i = 0; i < activeDocumentsModel.count; ++i) {
                activeDocumentsModel.setProperty(i, "status", "CANCELLED")
            }
        }
        reportClient.fetchReports()
    }

    function startProcessing() {
        if (!systemPoller.isConnected) {
            appendStatus("Cannot start processing: backend daemon is disconnected.")
            return
        }
        if (!modelDownloader.modelsReady) {
            appendStatus("[Action Required] AI models are not downloaded. Starting background download...")
            modelDownloader.startDownload()
            return
        }
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
        appendStatus("Queued " + paths.length + " document(s) for local on-device processing via Gemma 2 2B.")
        uploadClient.uploadDocuments(paths)
    }

    UploadClient {
        id: uploadClient
        onJobQueued: function(jobId) {
            currentJobId = jobId
            batchConfirmed = true
            appendStatus("Job #" + jobId + " queued with " + activeDocumentsModel.count + " document(s).")
            for (var i = 0; i < activeDocumentsModel.count; ++i) {
                activeDocumentsModel.setProperty(i, "status", "QUEUED")
            }
            documentQueueModel.clear()
            poller.startPolling(jobId)
            reportClient.fetchReports()
        }
        onUploadFailed: function(reason) {
            appendStatus("Upload failed: " + reason)
            batchConfirmed = false
        }
        onJobCancelled: function(jobId) {
            appendStatus("[Cancellation Confirmed] Job #" + jobId + " ingestion halted on server.")
            batchConfirmed = false
            for (var i = 0; i < activeDocumentsModel.count; ++i) {
                activeDocumentsModel.setProperty(i, "status", "CANCELLED")
            }
            reportClient.fetchReports()
        }
    }

    PipelineStatusPoller {
        id: poller
        onStatusUpdated: function(status, docs) {
            if (docs && docs.length > 0 && activeDocumentsModel.count > 0) {
                for (var i = 0; i < docs.length; ++i) {
                    var d = docs[i]
                    var targetIdx = -1
                    for (var m = 0; m < activeDocumentsModel.count; ++m) {
                        var existing = activeDocumentsModel.get(m)
                        if ((d.file_path && existing.filePath === d.file_path) ||
                            (d.file_name && existing.fileName === d.file_name)) {
                            targetIdx = m
                            break
                        }
                    }
                    if (targetIdx < 0 && i < activeDocumentsModel.count) {
                        targetIdx = i
                    }
                    if (targetIdx >= 0) {
                        if (d.chunk_count !== undefined) {
                            activeDocumentsModel.setProperty(targetIdx, "chunkCount", d.chunk_count)
                        }
                        if (d.page_count !== undefined) {
                            activeDocumentsModel.setProperty(targetIdx, "pageCount", d.page_count)
                        }
                        if (d.status !== undefined) {
                            activeDocumentsModel.setProperty(targetIdx, "status", d.status)
                        }
                    }
                }
            }
        }
        onCompleted: {
            poller.stopPolling()
            appendStatus("Ingestion complete. Vector embeddings persisted to local SQLite ('document_chunks') and indexed.")
            appendStatus("Backend advancing automatically to statutory extraction...")
            batchConfirmed = false
            for (var i = 0; i < activeDocumentsModel.count; ++i) {
                activeDocumentsModel.setProperty(i, "status", "INGESTED")
            }
            appWindow.isExtracting = true
            appWindow.extractionStep = 0
            appWindow.extractionTotal = 0
            appWindow.extractionCurrentGroup = "Initializing retrieval..."
            appWindow.extractionProgressPct = 0.0
            appWindow.missingMetricsSummary = []
            reportClient.fetchReports()
            reportClient.fetchDocuments(currentJobId)
            extractionPoller.startPolling(currentJobId)
        }
        onCancelled: {
            appendStatus("[Pipeline Cancelled] Job #" + currentJobId + " ingestion pipeline halted.")
            batchConfirmed = false
            for (var i = 0; i < activeDocumentsModel.count; ++i) {
                activeDocumentsModel.setProperty(i, "status", "CANCELLED")
            }
            reportClient.fetchReports()
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
            appWindow.extractionTotal = 0
            appWindow.extractionCurrentGroup = "Initializing retrieval..."
            appWindow.extractionProgressPct = 0.0
            appWindow.missingMetricsSummary = []
            appendStatus("Extraction initiated across statutory BRSR sections...")
            extractionPoller.startPolling(currentJobId)
        }
        onExtractionCancelled: function(jobId) {
            appWindow.isExtracting = false
            appendStatus("[Cancellation Confirmed] Job #" + jobId + " statutory extraction halted on server.")
            reportClient.fetchReports()
        }
        onRequestFailed: function(reason) {
            appWindow.isExtracting = false
            appendStatus("Extraction request failed: " + reason)
        }
    }

    SystemStatusPoller {
        id: systemPoller
        Component.onCompleted: startPolling()
        onStatusUpdated: {
            if (systemPoller.isConnected !== appWindow.lastKnownConnected) {
                appWindow.lastKnownConnected = systemPoller.isConnected
                if (!systemPoller.isConnected) {
                    appendStatus("Backend daemon disconnected.")
                } else {
                    appendStatus("Connected to carbon-engine daemon (" + systemPoller.activeModel + ").")
                }
            }
        }
    }
    Connections {
        target: backendModelDownloader
        function onDownloadCompleted() {
            appendStatus("All AI models have been downloaded and verified in local storage.")
            if (systemPoller) {
                systemPoller.startPolling()
            }
        }
        function onErrorChanged() {
            if (backendModelDownloader && backendModelDownloader.hasError) {
                appendStatus("[Model Download Error] " + backendModelDownloader.errorMessage)
            }
        }
    }

    readonly property var modelDownloader: backendModelDownloader
    readonly property var gitHubClient: (typeof backendGitHubClient !== "undefined") ? backendGitHubClient : null
    property alias systemPoller: systemPoller
    property bool lastKnownConnected: false

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
        onMissingMetricsUpdated: function(list) {
            appWindow.missingMetricsSummary = list
        }
        onMissingMetricsReceived: function(groupLabel, missingKeys) {
            var keyStr = (missingKeys && missingKeys.join) ? missingKeys.join(", ") : missingKeys
            appendStatus("[Notice] " + groupLabel + ": Missing from context (" + keyStr + ") - manual entry needed")
        }
        onCompleted: {
            extractionPoller.stopPolling()
            appWindow.isExtracting = false
            var totalStr = extractionTotal > 0 ? (extractionTotal + "/" + extractionTotal + " sections") : (extractionStep + " sections");
            appendStatus("Extraction complete (" + totalStr + " processed locally). Opening review screen...")
            reportClient.fetchReports()
            reviewScreen.openForJob(currentJobId)
        }
        onCancelled: {
            appWindow.isExtracting = false
            appendStatus("[Extraction Cancelled] Job #" + currentJobId + " extraction cancelled.")
            reportClient.fetchReports()
        }
        onFailed: function(reason) {
            appWindow.isExtracting = false
            appendStatus("Extraction failed: " + reason)
        }
    }

    ReportReviewScreen {
        id: reviewScreen
        onReportAccepted: function(jobId) {
            appendStatus("Report accepted and locked. Generating final HTML...")
            appWindow.isCurrentReportLocked = true
            reportClient.fetchReport(jobId)
            reportClient.fetchReports()
            if (gitHubClient) {
                gitHubClient.recordReportCompleted()
            }
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
                rawReportsMap[r.job_id] = r
                var docList = r.documents || []
                var isLocked = (r.report_locked === true || r.extraction_status === "ACCEPTED")
                reportsHistoryModel.append({
                    "jobId": r.job_id,
                    "title": r.title,
                    "status": r.status,
                    "extractionStatus": r.extraction_status,
                    "reportLocked": isLocked,
                    "date": r.date,
                    "docCount": docList.length,
                    "documents": docList
                })
            }
            appendStatus("Loaded " + reports.length + " report(s) from database.")
            if (reports.length > 0 && currentJobId === -1) {
                selectReport(reports[0].job_id)
            } else if (currentJobId !== -1) {
                for (var j = 0; j < reportsHistoryModel.count; ++j) {
                    if (reportsHistoryModel.get(j).jobId === currentJobId) {
                        activeReportTitle = reportsHistoryModel.get(j).title
                        isCurrentReportLocked = (reportsHistoryModel.get(j).reportLocked === true || reportsHistoryModel.get(j).extractionStatus === "ACCEPTED")
                        break
                    }
                }
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
                        "status": d.status || "INGESTED"
                    })
                }
            }
        }
        onReportDeleted: function(jobId) {
            if (rawReportsMap && rawReportsMap[jobId] !== undefined) {
                delete rawReportsMap[jobId]
            }
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

    SettingsModal {
        id: settingsModal
        poller: systemPoller
    }

    BugReportModal {
        id: bugReportModal
    }

    FeedbackModal {
        id: feedbackModal
    }

    FileDialog {
        id: addDocDialog
        title: "Select Documents for Ingestion"
        nameFilters: ["All Supported Documents (*.pdf *.xlsx *.txt *.png *.jpg *.jpeg)", "PDF Documents (*.pdf)", "Excel Spreadsheets (*.xlsx)", "Text Documents (*.txt)", "Image Documents (*.png *.jpg *.jpeg)"]
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
                var cleanPath = BrsrMeta.cleanFileUrl(raw)
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
        }
    }

    header: Rectangle {
        id: customTitleBar
        height: 38
        color: Theme.bgHeader
        border.color: Theme.borderCol
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
                    text: "Carbon ESG"
                    font.pixelSize: 12
                    font.bold: true
                    color: Theme.textBright
                }

                Label {
                    text: "•"
                    font.pixelSize: 10
                    color: Theme.borderSubtle
                }

                Label {
                    text: systemPoller.workloadTitle
                    font.pixelSize: 10
                    color: Theme.brandAccent
                    font.bold: true
                }

                Label {
                    text: "•"
                    font.pixelSize: 10
                    color: Theme.borderSubtle
                }

                Label {
                    text: "BRSR ESG Analytics"
                    font.pixelSize: 11
                    color: Theme.textMuted
                }
            }

            Item { Layout.fillWidth: true }

            // Centered Workspace / Report Status
            Label {
                text: appWindow.isExtracting
                    ? ("Extracting: " + (appWindow.extractionCurrentGroup !== "" ? appWindow.extractionCurrentGroup : "BRSR Sections"))
                    : (appWindow.activeReportTitle !== "" ? appWindow.activeReportTitle : "Workspace")
                font.pixelSize: 11
                color: Theme.textMuted
                elide: Text.ElideMiddle
                Layout.maximumWidth: 450
                Layout.alignment: Qt.AlignVCenter
            }

            Item { Layout.fillWidth: true }

            // Mode Status Badge (Informational offline indicator)
            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredHeight: 24
                Layout.preferredWidth: modeBadgeLayout.implicitWidth + 18
                radius: 12
                color: !systemPoller.isConnected ? Theme.badgeDangerBg : Theme.bgSubtle
                border.color: !systemPoller.isConnected ? Theme.badgeDangerBorder : Theme.borderCol
                border.width: 1

                RowLayout {
                    id: modeBadgeLayout
                    anchors.centerIn: parent
                    spacing: 6

                    Text {
                        text: !systemPoller.isConnected ? "●" : "⚡"
                        font.pixelSize: !systemPoller.isConnected ? 10 : 11
                        color: !systemPoller.isConnected ? Theme.badgeDangerText : (!modelDownloader.modelsReady ? Theme.warningColor : Theme.brandAccent)
                    }

                    Label {
                        text: {
                            if (!systemPoller.isConnected) return "Disconnected"
                            if (!modelDownloader.modelsReady) return "Models Required"
                            return systemPoller.activeModel + " • " + systemPoller.workloadTag
                        }
                        color: !systemPoller.isConnected ? Theme.badgeDangerText : (!modelDownloader.modelsReady ? Theme.warningColor : Theme.textBright)
                        font.pixelSize: 10
                        font.bold: true
                    }
                }
            }

            // Update Notification Pill in Titlebar
            Rectangle {
                visible: gitHubClient && gitHubClient.updateAvailable
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredHeight: 24
                Layout.preferredWidth: updatePillLayout.implicitWidth + 16
                radius: 12
                color: Theme.badgePillBlueBg
                border.color: Theme.brandAccent
                border.width: 1

                RowLayout {
                    id: updatePillLayout
                    anchors.centerIn: parent
                    spacing: 4
                    Text { text: "🚀"; font.pixelSize: 10 }
                    Label {
                        text: "Update v" + (gitHubClient ? gitHubClient.latestVersion : "")
                        color: Theme.badgePillBlueText
                        font.pixelSize: 10
                        font.bold: true
                    }
                }

                MouseArea {
                    id: updateMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: if (gitHubClient) gitHubClient.downloadAndInstallUpdate()
                }
                ToolTip.visible: updateMouse.containsMouse
                ToolTip.text: "New release available! Click to download & install."
                ToolTip.delay: 300
            }

            // Report Bug / Feedback Button
            Button {
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredHeight: 26
                Layout.preferredWidth: 32
                flat: true
                hoverEnabled: true
                background: Rectangle {
                    color: parent.hovered ? Theme.bgHover : (parent.pressed ? Theme.bgActive : "transparent")
                    border.color: parent.hovered ? Theme.borderSubtle : "transparent"
                    border.width: 1
                    radius: 4
                }
                contentItem: Text {
                    text: "🐞"
                    color: Theme.textMain
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                ToolTip.visible: hovered
                ToolTip.text: "Report Bug / File Issue"
                ToolTip.delay: 300
                onClicked: appWindow.openBugReportModal()
            }

            // Hardware / Testing Settings Toggle
            Button {
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredHeight: 26
                Layout.preferredWidth: 32
                flat: true
                hoverEnabled: true
                background: Rectangle {
                    color: parent.hovered ? Theme.bgHover : (parent.pressed ? Theme.bgActive : "transparent")
                    border.color: parent.hovered ? Theme.borderSubtle : "transparent"
                    border.width: 1
                    radius: 4
                }
                contentItem: Text {
                    text: "⚙"
                    color: Theme.textMain
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                ToolTip.visible: hovered
                ToolTip.text: "Hardware & Inference Testing Settings"
                onClicked: settingsModal.open()
            }

            // Theme Toggle
            Button {
                id: themeToggleBtn
                text: Theme.isDarkMode ? "🌙" : "☀️"
                flat: true
                hoverEnabled: true
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredHeight: 26
                Layout.preferredWidth: 32
                ToolTip.visible: hovered
                ToolTip.text: Theme.isDarkMode ? "Switch to Light Mode" : "Switch to Dark Mode"
                ToolTip.delay: 300
                background: Rectangle {
                    color: parent.hovered ? Theme.bgHover : (parent.pressed ? Theme.bgActive : "transparent")
                    border.color: parent.hovered ? Theme.borderSubtle : "transparent"
                    border.width: 1
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: Theme.isDarkMode = !Theme.isDarkMode
            }

            // Window Controls (Minimize, Maximize/Restore, Close)
            RowLayout {
                spacing: 0
                Layout.fillHeight: true

                // Minimize
                Rectangle {
                    Layout.preferredWidth: 46
                    Layout.fillHeight: true
                    color: minMouse.containsMouse ? Theme.bgHover : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "—"
                        font.pixelSize: 11
                        color: Theme.textMain
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
                    Layout.fillHeight: true
                    color: maxMouse.containsMouse ? Theme.bgHover : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: appWindow.visibility === Window.Maximized ? "🗗" : "🗖"
                        font.pixelSize: 12
                        color: Theme.textMain
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
                    Layout.fillHeight: true
                    color: closeMouse.containsMouse ? "#e81123" : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        font.pixelSize: 11
                        color: closeMouse.containsMouse ? "#ffffff" : Theme.textMain
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
        spacing: 0
        clip: true

        LeftSidebar {
            id: leftSidebar
            sidebarExpanded: appWindow.sidebarExpanded
            expandedWidth: appWindow.leftSidebarWidth
            historyModel: reportsHistoryModel
            currentJobId: appWindow.currentJobId

            onToggleExpanded: appWindow.sidebarExpanded = !appWindow.sidebarExpanded
            onReportSelected: function(jobId) {
                appWindow.selectReport(jobId)
            }
            onDeleteReportRequested: function(jobId) {
                reportClient.deleteReport(jobId)
            }
            onRefreshReportsRequested: reportClient.fetchReports()
        }

        // Left Splitter Handle
        Rectangle {
            id: leftSplitterHandle
            Layout.fillHeight: true
            Layout.preferredWidth: appWindow.sidebarExpanded ? 8 : 0
            visible: appWindow.sidebarExpanded
            color: "transparent"

            Rectangle {
                anchors.centerIn: parent
                width: 2
                height: 36
                radius: 1
                color: leftSplitterMouse.containsMouse || leftSplitterMouse.pressed ? Theme.brandAccent : Theme.borderSubtle
            }

            MouseArea {
                id: leftSplitterMouse
                anchors.fill: parent
                anchors.leftMargin: -3
                anchors.rightMargin: -3
                hoverEnabled: true
                cursorShape: Qt.SplitHCursor

                property real dragStartGlobalX: 0
                property real dragStartWidth: 0

                onPressed: function(mouse) {
                    var pt = mapToItem(appWindow.contentItem, mouse.x, 0)
                    dragStartGlobalX = pt.x
                    dragStartWidth = appWindow.leftSidebarWidth
                }
                onPositionChanged: function(mouse) {
                    if (pressed) {
                        var pt = mapToItem(appWindow.contentItem, mouse.x, 0)
                        var delta = pt.x - dragStartGlobalX
                        var newW = Math.max(180, Math.min(420, dragStartWidth + delta))
                        appWindow.leftSidebarWidth = newW
                    }
                }
            }
        }

        CentralWorkspace {
            id: centralWorkspace
        }

        // Right Splitter Handle
        Rectangle {
            id: rightSplitterHandle
            Layout.fillHeight: true
            Layout.preferredWidth: 8
            color: "transparent"

            Rectangle {
                anchors.centerIn: parent
                width: 2
                height: 36
                radius: 1
                color: rightSplitterMouse.containsMouse || rightSplitterMouse.pressed ? Theme.brandAccent : Theme.borderSubtle
            }

            MouseArea {
                id: rightSplitterMouse
                anchors.fill: parent
                anchors.leftMargin: -3
                anchors.rightMargin: -3
                hoverEnabled: true
                cursorShape: Qt.SplitHCursor

                property real dragStartGlobalX: 0
                property real dragStartWidth: 0

                onPressed: function(mouse) {
                    var pt = mapToItem(appWindow.contentItem, mouse.x, 0)
                    dragStartGlobalX = pt.x
                    dragStartWidth = appWindow.rightPanelWidth
                }
                onPositionChanged: function(mouse) {
                    if (pressed) {
                        var pt = mapToItem(appWindow.contentItem, mouse.x, 0)
                        var delta = dragStartGlobalX - pt.x // Dragging left increases width
                        var newW = Math.max(250, Math.min(500, dragStartWidth + delta))
                        appWindow.rightPanelWidth = newW
                    }
                }
            }
        }

        RightPanel {
            id: rightPanel
            panelWidth: appWindow.rightPanelWidth
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
