import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtWebEngine

Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.minimumWidth: 450
    color: appWindow.bgBase
    radius: 4
    border.color: appWindow.borderCol
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
                    color: (appWindow.isExtracting || appWindow.activeReportTitle !== "") ? appWindow.textBright : appWindow.textMuted
                }
                Label {
                    text: appWindow.isExtracting
                        ? ("Processing Section " + appWindow.extractionStep + " of " + appWindow.extractionTotal + " (" + Math.round(appWindow.extractionProgressPct) + "%)")
                        : (appWindow.activeReportTitle !== ""
                            ? ("Generated BRSR Report Preview" + (activeDocumentsModel.count > 0 ? (" • " + activeDocumentsModel.count + " document" + (activeDocumentsModel.count > 1 ? "s" : "")) : ""))
                            : "Standing by for document extraction")
                    font.pixelSize: 11
                    color: appWindow.isExtracting ? appWindow.brandAccent : appWindow.textMuted
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Review & Edit"
                visible: appWindow.currentJobId > 0 && !appWindow.isExtracting
                Layout.preferredHeight: 28
                background: Rectangle {
                    color: parent.down ? appWindow.bgActive
                         : (parent.hovered ? appWindow.bgHover : appWindow.bgSubtle)
                    border.color: appWindow.borderSubtle
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: appWindow.textMain
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
                         : (parent.down ? appWindow.bgActive
                         : (parent.hovered ? appWindow.bgHover : appWindow.bgSubtle))
                    border.color: appWindow.borderSubtle
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? appWindow.textMain : appWindow.textMuted
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
                    color: !parent.enabled ? appWindow.bgSubtle
                         : (parent.down ? appWindow.brandAccentPressed
                         : (parent.hovered ? appWindow.brandAccentHover : appWindow.brandAccent))
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? appWindow.textBright : appWindow.textMuted
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
                var path = selectedFile.toString()
                if (path.indexOf("file:///") === 0) {
                    path = path.substring(path.charAt(9) === ':' ? 8 : 7)
                } else if (path.indexOf("file://") === 0) {
                    path = path.substring(7)
                }
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
                var path = selectedFile.toString()
                if (path.indexOf("file:///") === 0) {
                    path = path.substring(path.charAt(9) === ':' ? 8 : 7)
                } else if (path.indexOf("file://") === 0) {
                    path = path.substring(7)
                }
                webView.printToPdf(path)
                appWindow.appendStatus("Exported PDF report to: " + path)
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: appWindow.borderCol }

        // Subtle Missing Metrics Notice Banner in Preview Mode
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            visible: !appWindow.isExtracting && appWindow.missingMetricsSummary.length > 0
            color: "#302400"
            border.color: appWindow.warningColor
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
                    color: appWindow.textBright
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }

                Button {
                    text: "Review & Fill In"
                    Layout.preferredHeight: 24
                    background: Rectangle {
                        color: parent.down ? appWindow.brandAccentPressed
                             : (parent.hovered ? appWindow.brandAccentHover : appWindow.brandAccent)
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.text
                        color: appWindow.textBright
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
                    color: appWindow.bgCard
                    radius: 6
                    border.color: appWindow.borderCol
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
                                color: appWindow.bgSubtle
                                border.color: appWindow.brandAccent
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
                                    text: "Local LLM Statutory Extraction"
                                    font.pixelSize: 14
                                    font.bold: true
                                    color: appWindow.textBright
                                }
                                Label {
                                    text: appWindow.extractionCurrentGroup !== ""
                                        ? ("Current Stage: " + appWindow.extractionCurrentGroup)
                                        : "Initializing retrieval and context synthesis..."
                                    font.pixelSize: 12
                                    color: appWindow.brandAccent
                                    elide: Text.ElideRight
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Label {
                                text: appWindow.extractionStep + " / " + appWindow.extractionTotal + " Sections (" + Math.round(appWindow.extractionProgressPct) + "%)"
                                font.pixelSize: 13
                                font.bold: true
                                color: appWindow.textMain
                            }
                        }

                        // Progress bar track
                        Rectangle {
                            Layout.fillWidth: true
                            height: 8
                            radius: 4
                            color: appWindow.bgSubtle

                            Rectangle {
                                id: progressBarFill
                                height: parent.height
                                radius: 4
                                color: appWindow.brandAccent
                                width: Math.max(4, parent.width * (Math.min(100.0, Math.max(0.0, appWindow.extractionProgressPct)) / 100.0))

                                Behavior on width {
                                    NumberAnimation { duration: 250; easing.type: Easing.OutCubic }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                text: "Local inference powered by llama.cpp GGUF runtime"
                                font.pixelSize: 11
                                color: appWindow.textMuted
                            }
                            Item { Layout.fillWidth: true }
                            Label {
                                text: "Streaming live section progress..."
                                font.pixelSize: 11
                                color: appWindow.textMuted
                            }
                        }
                    }
                }

                // Missing Metrics Panel
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: appWindow.bgCard
                    radius: 6
                    border.color: appWindow.borderCol
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
                                color: appWindow.textBright
                            }

                            Rectangle {
                                visible: appWindow.missingMetricsSummary.length > 0
                                radius: 10
                                color: "#3d2800"
                                border.color: appWindow.warningColor
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
                                    color: appWindow.warningColor
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Label {
                                text: "Cataloged in real time"
                                font.pixelSize: 11
                                color: appWindow.textMuted
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "Statutory fields not detected in source documents are listed below. You will be able to review and manually fill these in the Review Screen before finalizing the report."
                            font.pixelSize: 11
                            color: appWindow.textMuted
                            wrapMode: Text.WordWrap
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: appWindow.borderCol
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
                                    color: appWindow.textMuted
                                    Layout.alignment: Qt.AlignHCenter
                                }
                                Label {
                                    text: "No missing statutory metrics detected yet"
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: appWindow.textMuted
                                    Layout.alignment: Qt.AlignHCenter
                                }
                                Label {
                                    text: "As sections finish processing, any fields that require manual input will appear here."
                                    font.pixelSize: 11
                                    color: appWindow.textMuted
                                    Layout.alignment: Qt.AlignHCenter
                                }
                            }
                        }

                        // Scrollable list when missing metrics are detected
                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            visible: appWindow.missingMetricsSummary.length > 0
                            clip: true

                            ListView {
                                width: parent.width
                                spacing: 8
                                model: appWindow.missingMetricsSummary

                                delegate: Rectangle {
                                    width: ListView.view.width - 12
                                    implicitHeight: sectionCol.implicitHeight + 16
                                    color: appWindow.bgSubtle
                                    border.color: appWindow.borderSubtle
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
                                                color: appWindow.warningColor
                                            }

                                            Label {
                                                text: modelData.group
                                                font.pixelSize: 12
                                                font.bold: true
                                                color: appWindow.textBright
                                            }

                                            Item { Layout.fillWidth: true }

                                            Label {
                                                text: modelData.missing.length + " field(s) missing"
                                                font.pixelSize: 11
                                                color: appWindow.warningColor
                                            }
                                        }

                                        Flow {
                                            Layout.fillWidth: true
                                            spacing: 6

                                            Repeater {
                                                model: modelData.missing
                                                delegate: Rectangle {
                                                    radius: 3
                                                    color: appWindow.bgBase
                                                    border.color: appWindow.borderCol
                                                    border.width: 1
                                                    height: 22
                                                    width: fieldText.implicitWidth + 12

                                                    Text {
                                                        id: fieldText
                                                        anchors.centerIn: parent
                                                        text: modelData
                                                        font.pixelSize: 11
                                                        font.family: "Consolas", "Courier New", monospace
                                                        color: appWindow.warningColor
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
        }

        // Web Report Preview (Visible when not extracting)
        WebEngineView {
            id: webView
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !appWindow.isExtracting
            backgroundColor: appWindow.bgBase

            Component.onCompleted: {
                var loadingHtml = "<html><body style='background:#1e1e1e; color:#858585; font-family:\"Segoe UI\",system-ui,sans-serif; display:flex; flex-direction:column; align-items:center; justify-content:center; height:85vh; margin:0;'><div style='font-size:16px; margin-bottom:8px; color:#cccccc;'>No Report Loaded</div><div style='font-size:12px;'>Select a report from history or ingest documents to generate a report.</div></body></html>"
                loadHtml(loadingHtml, "http://localhost/");
            }
        }
    }

    Connections {
        target: appWindow
        function onActiveReportContentChanged() {
            if (appWindow.activeReportContent !== "") {
                webView.loadHtml(appWindow.activeReportContent, "http://localhost/");
            } else {
                var emptyHtml = "<html><body style='background:#1e1e1e; color:#858585; font-family:\"Segoe UI\",system-ui,sans-serif; display:flex; flex-direction:column; align-items:center; justify-content:center; height:85vh; margin:0;'><div style='font-size:16px; margin-bottom:8px; color:#cccccc;'>No Report Loaded</div><div style='font-size:12px;'>Select a report from history or ingest documents to generate a report.</div></body></html>"
                webView.loadHtml(emptyHtml, "http://localhost/");
            }
        }
    }
}