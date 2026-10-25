import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtWebEngine
import CarbonEngineClient
import "BRSRMetadata.js" as BrsrMeta

Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.minimumWidth: 450
    color: Theme.bgBase
    radius: 4
    border.color: Theme.borderCol
    border.width: 1
    clip: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 2
                Label {
                    text: appWindow.isExtracting
                        ? "Extracting Report Data..."
                        : (appWindow.activeReportTitle !== "" ? appWindow.activeReportTitle : "No Report Selected")
                    font.pixelSize: 15
                    font.bold: true
                    color: (appWindow.isExtracting || appWindow.activeReportTitle !== "") ? Theme.textBright : Theme.textMuted
                }
                Label {
                    text: appWindow.isExtracting
                        ? ("Processing Section " + appWindow.extractionStep + (appWindow.extractionTotal > 0 ? (" of " + appWindow.extractionTotal) : "") + " (" + Math.round(appWindow.extractionProgressPct) + "%)")
                        : (appWindow.activeReportTitle !== ""
                            ? ("Generated BRSR Report Preview" + (activeDocumentsModel.count > 0 ? (" • " + activeDocumentsModel.count + " document" + (activeDocumentsModel.count > 1 ? "s" : "")) : ""))
                            : "Standing by for document extraction")
                    font.pixelSize: 11
                    color: appWindow.isExtracting ? Theme.brandAccent : Theme.textMuted
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Review & Edit"
                visible: appWindow.currentJobId > 0 && !appWindow.isExtracting
                Layout.preferredHeight: 28
                background: Rectangle {
                    color: parent.down ? Theme.bgActive
                         : (parent.hovered ? Theme.bgHover : Theme.bgSubtle)
                    border.color: Theme.borderSubtle
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: Theme.textMain
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: appWindow.openReviewForCurrentJob()
            }

            Button {
                text: "Export HTML"
                enabled: !appWindow.isExtracting && appWindow.activeReportContent !== ""
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
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: saveHtmlDialog.open()
            }

            Button {
                text: "Export PDF"
                enabled: !appWindow.isExtracting && appWindow.activeReportContent !== ""
                Layout.preferredHeight: 28
                background: Rectangle {
                    color: !parent.enabled ? Theme.bgSubtle
                         : (parent.down ? Theme.brandAccentPressed
                         : (parent.hovered ? Theme.brandAccentHover : Theme.brandAccent))
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? Theme.textBright : Theme.textMuted
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: savePdfDialog.open()
            }
        }

        FileDialog {
            id: saveHtmlDialog
            title: "Export Report as HTML"
            nameFilters: ["HTML Files (*.html)"]
            fileMode: FileDialog.SaveFile
            defaultSuffix: "html"
            onAccepted: {
                var path = BrsrMeta.cleanFileUrl(selectedFile.toString())
                if (reportClient.saveHtmlToFile(path, appWindow.activeReportContent)) {
                    appWindow.appendStatus("Exported HTML report to: " + path)
                } else {
                    appWindow.appendStatus("Failed to export HTML report to: " + path)
                }
            }
        }

        FileDialog {
            id: savePdfDialog
            title: "Export Report as PDF"
            nameFilters: ["PDF Files (*.pdf)"]
            fileMode: FileDialog.SaveFile
            defaultSuffix: "pdf"
            onAccepted: {
                var path = BrsrMeta.cleanFileUrl(selectedFile.toString())
                if (webViewLoader.item) {
                    appWindow.appendStatus("Rendering PDF report to: " + path + "...")
                    webViewLoader.item.printToPdf(path)
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderCol }

        // Subtle Missing Metrics Notice Banner in Preview Mode
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            visible: !appWindow.isExtracting && appWindow.missingMetricsSummary.length > 0
            color: Theme.badgeWarningBg
            border.color: Theme.warningColor
            border.width: 1
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8

                Label {
                    text: "⚠️"
                    font.pixelSize: 13
                }

                Label {
                    Layout.fillWidth: true
                    text: {
                        var total = 0
                        for (var i = 0; i < appWindow.missingMetricsSummary.length; ++i) {
                            total += appWindow.missingMetricsSummary[i].missing.length
                        }
                        return total + " statutory metric(s) were not found in source documents and require manual entry."
                    }
                    color: Theme.textBright
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }

                Button {
                    text: "Review & Fill In"
                    Layout.preferredHeight: 24
                    background: Rectangle {
                        color: parent.down ? Theme.brandAccentPressed
                             : (parent.hovered ? Theme.brandAccentHover : Theme.brandAccent)
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.text
                        color: Theme.textBright
                        font.pixelSize: 11
                        font.weight: Font.Medium
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: appWindow.openReviewForCurrentJob()
                }
            }
        }

        // Live Extraction Dashboard (Active during LLM extraction)
        Item {
            id: extractionProgressView
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: appWindow.isExtracting

            ColumnLayout {
                anchors.fill: parent
                spacing: 14

                // Extraction Status & Progress Card
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 110
                    color: Theme.bgCard
                    radius: 6
                    border.color: Theme.borderCol
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Rectangle {
                                width: 28
                                height: 28
                                radius: 14
                                color: Theme.bgSubtle
                                border.color: Theme.brandAccent
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: "⚡"
                                    font.pixelSize: 14
                                }
                            }

                            ColumnLayout {
                                spacing: 2
                                Label {
                                    text: "Local LLM Statutory Extraction (Gemma 2 2B / BGE-Small)"
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: Theme.textBright
                                }
                                Label {
                                    text: appWindow.extractionCurrentGroup !== ""
                                        ? ("Current Stage: " + appWindow.extractionCurrentGroup)
                                        : "Initializing retrieval and context synthesis..."
                                    font.pixelSize: 12
                                    color: Theme.brandAccent
                                    elide: Text.ElideRight
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Label {
                                text: appWindow.extractionStep + (appWindow.extractionTotal > 0 ? (" / " + appWindow.extractionTotal) : "") + " Sections (" + Math.round(appWindow.extractionProgressPct) + "%)"
                                font.pixelSize: 13
                                font.bold: true
                                color: Theme.textMain
                            }

                            Button {
                                id: cancelExtractionBtn
                                text: "✕ Cancel Job"
                                Layout.preferredHeight: 26
                                background: Rectangle {
                                    color: cancelExtractionBtn.down ? "#991b1b"
                                        : (cancelExtractionBtn.hovered ? "#dc2626" : Theme.badgeDangerBg)
                                    border.color: Theme.badgeDangerBorder
                                    border.width: 1
                                    radius: 3
                                }
                                contentItem: Text {
                                    text: cancelExtractionBtn.text
                                    color: cancelExtractionBtn.hovered ? "#ffffff" : Theme.badgeDangerText
                                    font.pixelSize: 11
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                ToolTip.visible: cancelExtractionBtn.hovered
                                ToolTip.delay: 300
                                ToolTip.text: "Halt statutory extraction and cancel local LLM worker."
                                onClicked: {
                                    appWindow.cancelActiveJob()
                                }
                            }
                        }

                        // Progress bar track
                        Rectangle {
                            Layout.fillWidth: true
                            height: 8
                            radius: 4
                            color: Theme.bgSubtle

                            Rectangle {
                                id: progressBarFill
                                height: parent.height
                                radius: 4
                                color: Theme.brandAccent
                                width: Math.max(4, parent.width * (Math.min(100.0, Math.max(0.0, appWindow.extractionProgressPct)) / 100.0))

                                Behavior on width {
                                    NumberAnimation { duration: 250; easing.type: Easing.OutCubic }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                text: "Local inference powered by llama.cpp GGUF runtime (gemma-2-2b-it)"
                                font.pixelSize: 11
                                color: Theme.textMuted
                            }
                            Item { Layout.fillWidth: true }
                            Label {
                                text: "Streaming live section progress..."
                                font.pixelSize: 11
                                color: Theme.textMuted
                            }
                        }
                    }
                }

                // Missing Metrics Panel
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: Theme.bgCard
                    radius: 6
                    border.color: Theme.borderCol
                    border.width: 1
                    clip: true

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                text: "📋"
                                font.pixelSize: 14
                            }

                            Label {
                                text: "Missing Statutory Metrics (Manual Input Queue)"
                                font.pixelSize: 13
                                font.bold: true
                                color: Theme.textBright
                            }

                            Rectangle {
                                visible: appWindow.missingMetricsSummary.length > 0
                                radius: 10
                                color: Theme.badgeWarningDarkBg
                                border.color: Theme.warningColor
                                border.width: 1
                                Layout.preferredHeight: 20
                                Layout.preferredWidth: countText.implicitWidth + 12

                                Text {
                                    id: countText
                                    anchors.centerIn: parent
                                    text: {
                                        var total = 0
                                        for (var i = 0; i < appWindow.missingMetricsSummary.length; ++i) {
                                            total += appWindow.missingMetricsSummary[i].missing.length
                                        }
                                        return total + " unlocated"
                                    }
                                    font.pixelSize: 10
                                    font.bold: true
                                    color: Theme.warningColor
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Label {
                                text: "Cataloged in real time"
                                font.pixelSize: 11
                                color: Theme.textMuted
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "Statutory fields not detected in source documents are listed below. You will be able to review and manually fill these in the Review Screen before finalizing the report."
                            font.pixelSize: 11
                            color: Theme.textMuted
                            wrapMode: Text.WordWrap
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: Theme.borderCol
                        }

                        // Placeholder when no missing metrics yet
                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            visible: appWindow.missingMetricsSummary.length === 0

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 6

                                Label {
                                    text: "✓"
                                    font.pixelSize: 22
                                    color: Theme.textMuted
                                    Layout.alignment: Qt.AlignHCenter
                                }
                                Label {
                                    text: "No missing statutory metrics detected yet"
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: Theme.textMuted
                                    Layout.alignment: Qt.AlignHCenter
                                }
                                Label {
                                    text: "As sections finish processing, any fields that require manual input will appear here."
                                    font.pixelSize: 11
                                    color: Theme.textMuted
                                    Layout.alignment: Qt.AlignHCenter
                                }
                            }
                        }

                        // Missing metrics list with direct scrollbar
                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            visible: appWindow.missingMetricsSummary.length > 0
                            clip: true
                            spacing: 8
                            model: appWindow.missingMetricsSummary
                            boundsBehavior: Flickable.StopAtBounds

                            ScrollBar.vertical: ScrollBar {
                                policy: ScrollBar.AsNeeded
                            }

                            delegate: Rectangle {
                                width: ListView.view.width - 12
                                implicitHeight: sectionCol.implicitHeight + 16
                                color: Theme.bgSubtle
                                border.color: Theme.borderSubtle
                                border.width: 1
                                radius: 4

                                ColumnLayout {
                                    id: sectionCol
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 10
                                    spacing: 8

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 6

                                        Rectangle {
                                            width: 6
                                            height: 6
                                            radius: 3
                                            color: Theme.warningColor
                                        }

                                        Label {
                                            text: modelData.group
                                            font.pixelSize: 12
                                            font.bold: true
                                            color: Theme.textBright
                                        }

                                        Item { Layout.fillWidth: true }

                                        Label {
                                            text: modelData.missing.length + " field(s) missing"
                                            font.pixelSize: 11
                                            color: Theme.warningColor
                                        }
                                    }

                                    Flow {
                                        Layout.fillWidth: true
                                        spacing: 6

                                        Repeater {
                                            model: modelData.missing
                                            delegate: Rectangle {
                                                radius: 3
                                                color: Theme.bgBase
                                                border.color: Theme.borderCol
                                                border.width: 1
                                                height: 22
                                                width: fieldText.implicitWidth + 12

                                                Text {
                                                    id: fieldText
                                                    anchors.centerIn: parent
                                                    text: modelData
                                                    font.pixelSize: 11
                                                    font.family: "Consolas, monospace"
                                                    color: Theme.warningColor
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Web Report Preview (Loaded on-demand when active report exists)
        Loader {
            id: webViewLoader
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !appWindow.isExtracting
            active: !appWindow.isExtracting && (appWindow.activeReportContent !== "" || appWindow.currentJobId > 0)

            sourceComponent: WebEngineView {
                id: webView
                anchors.fill: parent
                backgroundColor: Theme.bgBase

                onPdfPrintingFinished: function(filePath, success) {
                    if (success) {
                        appWindow.appendStatus("Exported PDF report to: " + filePath)
                    } else {
                        appWindow.appendStatus("Failed to export PDF report to: " + filePath)
                    }
                }

                Component.onCompleted: {
                    var baseUrl = reportClient.baseUrl();
                    if (appWindow.activeReportContent !== "") {
                        loadHtml(appWindow.activeReportContent, baseUrl);
                    } else {
                        var loadingHtml = "<html><body style='background:#1e1e1e; color:#858585; font-family:\"Segoe UI\",system-ui,sans-serif; display:flex; flex-direction:column; align-items:center; justify-content:center; height:85vh; margin:0;'><div style='font-size:16px; margin-bottom:8px; color:#cccccc;'>No Report Loaded</div><div style='font-size:12px;'>Select a report from history or ingest documents to generate a report.</div></body></html>"
                        loadHtml(loadingHtml, baseUrl);
                    }
                }
            }
        }
    }

    Connections {
        target: appWindow
        function onActiveReportContentChanged() {
            if (webViewLoader.item) {
                var baseUrl = reportClient.baseUrl();
                if (appWindow.activeReportContent !== "") {
                    webViewLoader.item.loadHtml(appWindow.activeReportContent, baseUrl);
                } else {
                    var emptyHtml = "<html><body style='background:#1e1e1e; color:#858585; font-family:\"Segoe UI\",system-ui,sans-serif; display:flex; flex-direction:column; align-items:center; justify-content:center; height:85vh; margin:0;'><div style='font-size:16px; margin-bottom:8px; color:#cccccc;'>No Report Loaded</div><div style='font-size:12px;'>Select a report from history or ingest documents to generate a report.</div></body></html>"
                    webViewLoader.item.loadHtml(emptyHtml, baseUrl);
                }
            }
        }
    }
}
