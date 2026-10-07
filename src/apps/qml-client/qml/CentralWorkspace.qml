import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtWebEngine

Rectangle {
    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.minimumWidth: 450
    color: appWindow.bgCard
    radius: 8
    border.color: appWindow.borderCol
    border.width: 1
    clip: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Label { text: appWindow.activeReportTitle; font.pixelSize: 18; font.bold: true; color: appWindow.textMain }
            Item { Layout.fillWidth: true }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: appWindow.borderCol }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Label { text: "Edit generated report"; font.pixelSize: 13; color: appWindow.textMuted; font.bold: true }
            Item { Layout.fillWidth: true }
            
            Button { 
                text: "Export HTML"
                background: Rectangle { 
                    color: parent.down ? appWindow.bgSubtle : "transparent"
                    border.color: appWindow.brandAccent
                    border.width: 1
                    radius: 6
                }
                contentItem: Text {
                    text: parent.text
                    color: appWindow.brandAccent
                    font.pixelSize: 13
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: appWindow.appendStatus("HTML export pending...") 
            }
            
            Button { 
                text: "Export PDF"
                background: Rectangle { 
                    color: parent.down ? Qt.darker(appWindow.brandAccent, 1.2) : appWindow.brandAccent
                    radius: 6
                }
                contentItem: Text {
                    text: parent.text
                    color: "#0a192f"
                    font.pixelSize: 13
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: webView.printToPdf("exported_report.pdf") 
            }
        }

        WebEngineView {
            id: webView
            Layout.fillWidth: true
            Layout.fillHeight: true
            backgroundColor: appWindow.bgCard
            
            Component.onCompleted: {
                var loadingHtml = `<html><body style='color:${appWindow.brandAccent}; font-family:sans-serif; display:flex; align-items:center; justify-content:center; height:90vh; margin:0;'>Standing by for extraction...</body></html>`
                loadHtml(loadingHtml, "http://localhost/");
            }
        }
    }

    Connections {
        target: appWindow
        function onActiveReportContentChanged() {
            if (appWindow.activeReportContent !== "") {
                webView.loadHtml(appWindow.activeReportContent, "http://localhost/");
            }
        }
    }
}