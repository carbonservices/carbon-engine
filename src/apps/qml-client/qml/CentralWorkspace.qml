import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWebEngine

Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.minimumWidth: 450 // <-- Prevents the layout collapse seen in your screenshot
    color: appWindow.bgCard
    radius: 8
    border.color: appWindow.borderCol
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Label { text: appWindow.activeReportTitle; font.pixelSize: 17; font.bold: true; color: appWindow.textMain }
            Item { Layout.fillWidth: true }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: appWindow.borderCol }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Label { text: "Edit generated report"; font.pixelSize: 13; color: appWindow.textMuted; font.bold: true }
            Item { Layout.fillWidth: true }
            Button { text: "Export HTML"; onClicked: appWindow.appendStatus("HTML export pending...") }
            Button { text: "Export PDF"; highlighted: true; onClicked: webView.printToPdf("exported_report.pdf") }
        }

        // WebEngineView handles your template.html and Chart.js natively
        WebEngineView {
            id: webView
            Layout.fillWidth: true
            Layout.fillHeight: true
            backgroundColor: appWindow.isDarkMode ? "#14171a" : "#fdfdfd"
            
            // Render a placeholder until the backend returns the HTML
            Component.onCompleted: {
                loadHtml("<html><body style='color:#888; font-family:sans-serif;'>Standing by for extraction...</body></html>", "http://localhost/");
            }
        }
    }

    // Listen for the HTML payload from Main.qml and render it
    Connections {
        target: appWindow
        function onActiveReportContentChanged() {
            if (appWindow.activeReportContent !== "") {
                webView.loadHtml(appWindow.activeReportContent, "http://localhost/");
            }
        }
    }
}