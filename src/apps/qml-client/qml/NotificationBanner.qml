import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CarbonEngineClient

Rectangle {
    id: root

    property var client: (typeof appWindow !== "undefined" && appWindow.gitHubClient) ? appWindow.gitHubClient : (typeof backendGitHubClient !== "undefined" ? backendGitHubClient : null)

    signal openFeedbackRequested()
    signal openBugReportRequested()

    readonly property bool isUpdateMode: client ? client.shouldPromptUpdate : false
    readonly property bool isFeedbackMode: client ? (client.shouldPromptFeedback && !isUpdateMode) : false
    readonly property bool isVisible: isUpdateMode || isFeedbackMode

    visible: opacity > 0.01
    opacity: isVisible ? 1.0 : 0.0
    Layout.fillWidth: true
    implicitHeight: isVisible ? contentColumn.implicitHeight + 20 : 0

    Behavior on opacity { NumberAnimation { duration: 250; easing.type: Easing.InOutQuad } }
    Behavior on implicitHeight { NumberAnimation { duration: 250; easing.type: Easing.InOutQuad } }

    radius: 6
    clip: true

    color: isUpdateMode
        ? (Theme.isDarkMode ? "#13233a" : "#ebf5ff")
        : (Theme.isDarkMode ? "#2b220d" : "#fef9c3")

    border.color: isUpdateMode
        ? (Theme.isDarkMode ? "#2563eb" : "#3b82f6")
        : (Theme.isDarkMode ? "#ca8a04" : "#eab308")
    border.width: 1

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            // Badge Icon
            Rectangle {
                width: 34
                height: 34
                radius: 17
                color: root.isUpdateMode
                    ? (Theme.isDarkMode ? "#1e3a8a" : "#dbeafe")
                    : (Theme.isDarkMode ? "#451a03" : "#fef08a")

                Text {
                    anchors.centerIn: parent
                    text: root.isUpdateMode ? "🚀" : "⭐"
                    font.pixelSize: 16
                }
            }

            // Message Texts
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    text: root.isUpdateMode
                        ? ("New Release Available: Carbon ESG v" + (root.client ? root.client.latestVersion : ""))
                        : "How are you finding Carbon ESG?"
                    font.bold: true
                    font.pixelSize: 13
                    color: Theme.textBright
                }

                Label {
                    text: root.isUpdateMode
                        ? ("A new version is available for download with performance enhancements and bug fixes.")
                        : "Your feedback helps shape future releases. Would you like to share a rating or report feedback?"
                    font.pixelSize: 11
                    color: Theme.textMain
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            // Action Buttons
            RowLayout {
                spacing: 8
                Layout.alignment: Qt.AlignVCenter

                // Update Actions
                Button {
                    visible: root.isUpdateMode
                    text: "⬇️ Install Update"
                    highlighted: true
                    Layout.preferredHeight: 28
                    background: Rectangle {
                        color: parent.down ? Theme.brandAccentPressed : (parent.hovered ? Theme.brandAccentHover : Theme.brandAccent)
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "#ffffff"
                        font.pixelSize: 11
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        if (root.client) root.client.downloadAndInstallUpdate()
                    }
                }

                Button {
                    visible: root.isUpdateMode
                    text: "Release Notes"
                    flat: true
                    Layout.preferredHeight: 28
                    background: Rectangle {
                        color: parent.hovered ? Theme.bgHover : "transparent"
                        border.color: Theme.borderSubtle
                        border.width: 1
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        color: Theme.textMain
                        font.pixelSize: 11
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        if (root.client) root.client.openReleaseInBrowser()
                    }
                }

                Button {
                    visible: root.isUpdateMode
                    text: "Dismiss"
                    flat: true
                    Layout.preferredHeight: 28
                    contentItem: Text {
                        text: parent.text
                        color: Theme.textMuted
                        font.pixelSize: 11
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        if (root.client) root.client.dismissCurrentUpdate()
                    }
                }

                // Feedback Actions
                Button {
                    visible: root.isFeedbackMode
                    text: "⭐ Rate & Review"
                    highlighted: true
                    Layout.preferredHeight: 28
                    background: Rectangle {
                        color: parent.down ? "#b45309" : (parent.hovered ? "#d97706" : "#eab308")
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "#000000"
                        font.pixelSize: 11
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: root.openFeedbackRequested()
                }

                Button {
                    visible: root.isFeedbackMode
                    text: "Remind Me Later"
                    flat: true
                    Layout.preferredHeight: 28
                    background: Rectangle {
                        color: parent.hovered ? Theme.bgHover : "transparent"
                        border.color: Theme.borderSubtle
                        border.width: 1
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        color: Theme.textMain
                        font.pixelSize: 11
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        if (root.client) root.client.snoozeFeedback(7)
                    }
                }

                Button {
                    visible: root.isFeedbackMode
                    text: "Don't Ask Again"
                    flat: true
                    Layout.preferredHeight: 28
                    contentItem: Text {
                        text: parent.text
                        color: Theme.textMuted
                        font.pixelSize: 11
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        if (root.client) root.client.dismissFeedbackForever()
                    }
                }
            }
        }
    }
}

