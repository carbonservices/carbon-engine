import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CarbonEngineClient

Popup {
    id: root
    modal: true
    focus: true
    anchors.centerIn: parent
    width: Math.min(540, parent.width - 40)
    height: Math.min(520, parent.height - 40)
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var client: (typeof appWindow !== "undefined" && appWindow.gitHubClient) ? appWindow.gitHubClient : (typeof backendGitHubClient !== "undefined" ? backendGitHubClient : null)

    property int selectedRating: 5
    property int hoveredRating: 0

    background: Rectangle {
        color: Theme.bgMain
        radius: 8
        border.color: Theme.borderCol
        border.width: 1

        Rectangle {
            z: -1
            anchors.fill: parent
            anchors.margins: -4
            radius: 12
            color: "#000000"
            opacity: 0.3
        }
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: "⭐"
                font.pixelSize: 20
            }

            ColumnLayout {
                spacing: 2
                Label {
                    text: "Rate & Share Feedback"
                    font.bold: true
                    font.pixelSize: 15
                    color: Theme.textBright
                }
                Label {
                    text: "Help us enhance Carbon ESG with your experience and suggestions."
                    font.pixelSize: 11
                    color: Theme.textMuted
                }
            }

            Item { Layout.fillWidth: true }

            // Close button
            Rectangle {
                width: 24; height: 24; radius: 3
                color: closeMouse.containsMouse ? Theme.bgHover : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    font.pixelSize: 12
                    color: closeMouse.containsMouse ? Theme.textBright : Theme.textMuted
                }

                MouseArea {
                    id: closeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderCol }

        // Star Rating Selection
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 6

            Label {
                text: "How would you rate Carbon ESG?"
                font.pixelSize: 12
                font.bold: true
                color: Theme.textMain
                Layout.alignment: Qt.AlignHCenter
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 8

                Repeater {
                    model: 5
                    delegate: Item {
                        width: 38
                        height: 38

                        property int starIndex: index + 1
                        property bool isLit: root.hoveredRating > 0 ? (starIndex <= root.hoveredRating) : (starIndex <= root.selectedRating)

                        Text {
                            anchors.centerIn: parent
                            text: parent.isLit ? "⭐" : "☆"
                            font.pixelSize: 26
                            color: parent.isLit ? "#eab308" : Theme.textMuted
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onEntered: root.hoveredRating = parent.starIndex
                            onExited: root.hoveredRating = 0
                            onClicked: root.selectedRating = parent.starIndex
                        }
                    }
                }
            }

            Label {
                id: ratingLabel
                text: {
                    var r = root.hoveredRating > 0 ? root.hoveredRating : root.selectedRating
                    switch (r) {
                        case 1: return "1 - Poor (Needs substantial improvement)"
                        case 2: return "2 - Fair (Met some expectations)"
                        case 3: return "3 - Good (Satisfactory ESG extraction)"
                        case 4: return "4 - Very Good (Smooth & fast experience)"
                        case 5: return "5 - Excellent (Outstanding local AI performance!)"
                        default: return ""
                    }
                }
                font.pixelSize: 11
                color: Theme.brandAccent
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
        }

        // Category Selection
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                text: "Feedback Category"
                font.pixelSize: 11
                font.bold: true
                color: Theme.textMain
            }

            ComboBox {
                id: categoryCombo
                Layout.fillWidth: true
                model: [
                    "General User Experience",
                    "AI Model Extraction Accuracy & Speed",
                    "BRSR Report Editing & Export",
                    "Performance & Hardware Acceleration",
                    "Feature Request / Suggestion",
                    "Other"
                ]
                background: Rectangle {
                    color: Theme.bgInput
                    border.color: Theme.borderSubtle
                    border.width: 1
                    radius: 4
                }
            }
        }

        // Comments Input
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4

            Label {
                text: "Your Comments & Suggestions"
                font.pixelSize: 11
                font.bold: true
                color: Theme.textMain
            }

            TextArea {
                id: commentsInput
                Layout.fillWidth: true
                Layout.fillHeight: true
                placeholderText: "Tell us what you liked, what can be improved, or any features you'd like to see added..."
                wrapMode: Text.WordWrap
                color: Theme.textBright
                background: Rectangle {
                    color: Theme.bgInput
                    border.color: commentsInput.activeFocus ? Theme.brandAccent : Theme.borderSubtle
                    border.width: 1
                    radius: 4
                }
            }
        }

        // Anonymous hardware specs toggle
        CheckBox {
            id: attachSpecsCheck
            text: "Include anonymous hardware specifications (GPU, CPU, RAM) to assist optimization"
            checked: true
        }

        // Status Message Banner
        Rectangle {
            visible: root.client && (root.client.hasError || root.client.statusMessage !== "")
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            radius: 4
            color: (root.client && root.client.hasError) ? Theme.badgeDangerBg : Theme.badgeSuccessBg
            border.color: (root.client && root.client.hasError) ? Theme.badgeDangerBorder : Theme.successColor
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 6

                Text {
                    text: (root.client && root.client.hasError) ? "⚠️" : "✅"
                    font.pixelSize: 12
                }

                Label {
                    Layout.fillWidth: true
                    text: (root.client && root.client.hasError) ? root.client.errorMessage : (root.client ? root.client.statusMessage : "")
                    color: (root.client && root.client.hasError) ? Theme.badgeDangerText : Theme.successColor
                    font.pixelSize: 11
                    elide: Text.ElideRight
                }

                Button {
                    visible: root.client && root.client.lastIssueUrl !== ""
                    text: "View Issue"
                    flat: true
                    Layout.preferredHeight: 22
                    onClicked: {
                        if (root.client && root.client.lastIssueUrl !== "") {
                            Qt.openUrlExternally(root.client.lastIssueUrl)
                        }
                    }
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderCol }

        // Action Buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Item { Layout.fillWidth: true }

            Button {
                text: "Later"
                flat: true
                Layout.preferredHeight: 30
                onClicked: {
                    if (root.client) root.client.snoozeFeedback(7)
                    root.close()
                }
            }

            Button {
                text: "🌐 Open on GitHub.com"
                flat: true
                Layout.preferredHeight: 30
                background: Rectangle {
                    color: parent.hovered ? Theme.bgHover : "transparent"
                    border.color: Theme.borderSubtle
                    border.width: 1
                    radius: 4
                }
                onClicked: {
                    if (root.client) {
                        root.client.openFeedbackInBrowser(
                            root.selectedRating,
                            categoryCombo.currentText,
                            commentsInput.text,
                            attachSpecsCheck.checked
                        )
                    }
                    root.close()
                }
            }

            Button {
                text: "Submit Feedback"
                highlighted: true
                Layout.preferredHeight: 30
                background: Rectangle {
                    color: parent.down ? Theme.brandAccentPressed : (parent.hovered ? Theme.brandAccentHover : Theme.brandAccent)
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    if (root.client) {
                        root.client.submitFeedback(
                            root.selectedRating,
                            categoryCombo.currentText,
                            commentsInput.text,
                            attachSpecsCheck.checked
                        )
                    }
                    root.close()
                }
            }
        }
    }
}

