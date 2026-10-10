import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Enterprise ESG Report Data Review Modal
// Provides a tabbed review interface across Section A, Section B, Principles 1-9, and BRSR Core.
// Allows ESG consultants to inspect and edit extracted scalar data, narratives, and tabular records.
Dialog {
    id: reviewDialog
    title: ""
    modal: true
    width: Math.min(parent ? parent.width - 40 : 1240, 1240)
    height: Math.min(parent ? parent.height - 40 : 820, 820)
    anchors.centerIn: parent
    standardButtons: Dialog.NoButton
    closePolicy: Popup.NoAutoClose
    padding: 0

    background: Rectangle {
        color: appWindow.bgBase
        border.color: appWindow.borderCol
        border.width: 1
        radius: 6
    }

    property int jobId: -1
    property var sectionsByGroupId: ({})
    property var warningsByGroupId: ({})
    property bool accepted: false

    signal reportAccepted(int jobId)

    readonly property var sectionOrder: ["A", "B", "P1", "P2", "P3", "P4", "P5", "P6", "P7", "P8", "P9", "CORE"]
    readonly property var sectionLabels: ({
        "A": "Section A", "B": "Section B", "P1": "Principle 1", "P2": "Principle 2",
        "P3": "Principle 3", "P4": "Principle 4", "P5": "Principle 5", "P6": "Principle 6",
        "P7": "Principle 7", "P8": "Principle 8", "P9": "Principle 9", "CORE": "BRSR Core"
    })

    ExtractionClient {
        id: extractionClient
        onSectionsReady: function(sections, warnings) {
            var byGroup = {}
            for (var i = 0; i < sections.length; ++i) {
                byGroup[sections[i].group_id] = sections[i]
            }
            reviewDialog.sectionsByGroupId = byGroup

            var warningsByGroup = {}
            for (var w = 0; w < warnings.length; ++w) {
                var text = warnings[w]
                var groupId = text.split(":")[0]
                if (!warningsByGroup[groupId]) warningsByGroup[groupId] = []
                warningsByGroup[groupId].push(text)
            }
            reviewDialog.warningsByGroupId = warningsByGroup
        }
        onUpdateSucceeded: function(groupId) {
            appWindow.appendStatus("Saved edits for " + groupId)
        }
        onAcceptSucceeded: function() {
            reviewDialog.accepted = true
            reviewDialog.reportAccepted(reviewDialog.jobId)
            reviewDialog.close()
        }
        onRequestFailed: function(reason) {
            appWindow.appendStatus("Review action failed: " + reason)
        }
    }

    function groupsForSection(sectionId) {
        var result = []
        for (var groupId in sectionsByGroupId) {
            if (sectionsByGroupId[groupId].section === sectionId) {
                result.push(sectionsByGroupId[groupId])
            }
        }
        return result
    }

    function openForJob(id) {
        jobId = id
        accepted = false
        extractionClient.fetchSections(id)
        open()
    }

    onOpened: sectionTabBar.currentIndex = 0

    contentItem: ColumnLayout {
        spacing: 0

        // Custom Modal Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: appWindow.bgHeader
            border.color: appWindow.borderCol
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                ColumnLayout {
                    spacing: 2
                    Label {
                        text: "Review Extracted Report Data"
                        font.bold: true
                        font.pixelSize: 15
                        color: appWindow.textBright
                    }
                    Label {
                        text: "Verify and calibrate ESG metrics and disclosure tables before final report generation and audit lock."
                        font.pixelSize: 11
                        color: appWindow.textMuted
                    }
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    Layout.preferredHeight: 22
                    Layout.preferredWidth: jobPillText.implicitWidth + 14
                    color: "#1e3a5f"
                    radius: 11
                    border.color: "#3b82f6"
                    border.width: 1

                    Text {
                        id: jobPillText
                        anchors.centerIn: parent
                        text: "Job #" + reviewDialog.jobId
                        color: "#93c5fd"
                        font.pixelSize: 11
                        font.bold: true
                    }
                }

                Button {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    background: Rectangle {
                        color: parent.down ? appWindow.bgActive : (parent.hovered ? appWindow.bgHover : "transparent")
                        radius: 4
                    }
                    contentItem: Text {
                        text: "✕"
                        color: appWindow.textMuted
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: reviewDialog.close()
                }
            }
        }

        // Section Tabs Bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            color: appWindow.bgSidebar
            border.color: appWindow.borderCol
            border.width: 1

            TabBar {
                id: sectionTabBar
                anchors.fill: parent
                background: Rectangle { color: "transparent" }

                Repeater {
                    model: reviewDialog.sectionOrder
                    delegate: TabButton {
                        id: tabBtn
                        text: reviewDialog.sectionLabels[modelData]
                        Layout.fillHeight: true
                        font.pixelSize: 11
                        font.weight: tabBtn.checked ? Font.DemiBold : Font.Normal

                        background: Rectangle {
                            color: tabBtn.checked ? appWindow.bgBase : (tabBtn.hovered ? appWindow.bgHover : "transparent")
                            Rectangle {
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                height: 2
                                color: tabBtn.checked ? appWindow.brandAccent : "transparent"
                            }
                        }

                        contentItem: Text {
                            text: tabBtn.text
                            color: tabBtn.checked ? appWindow.textBright : (tabBtn.hovered ? appWindow.textMain : appWindow.textMuted)
                            font.pixelSize: 11
                            font.bold: tabBtn.checked
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }
        }

        // Scrollable Body for Cards
        StackLayout {
            id: sectionStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 12
            currentIndex: sectionTabBar.currentIndex

            Repeater {
                model: reviewDialog.sectionOrder
                delegate: ScrollView {
                    clip: true
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded

                    ColumnLayout {
                        width: sectionStack.width - 24
                        spacing: 14

                        Repeater {
                            model: reviewDialog.groupsForSection(modelData)
                            delegate: FieldGroupCard {
                                Layout.fillWidth: true
                                groupData: modelData
                                warnings: reviewDialog.warningsByGroupId[modelData.group_id] || []
                                locked: reviewDialog.accepted
                                onFieldEdited: function(groupId, data) {
                                    extractionClient.updateSection(reviewDialog.jobId, groupId, data)
                                }
                            }
                        }

                        Item { Layout.preferredHeight: 16 }
                    }
                }
            }
        }

        // Action Footer
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            color: appWindow.bgHeader
            border.color: appWindow.borderCol
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                Label {
                    Layout.fillWidth: true
                    text: reviewDialog.accepted
                        ? "✓ Report verified, accepted, and locked in the audit ledger."
                        : "All edits are automatically saved to draft. Click 'Accept & Lock' to finalize."
                    color: reviewDialog.accepted ? appWindow.successColor : appWindow.textMuted
                    font.pixelSize: 11
                }

                Button {
                    text: "Close"
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 80
                    background: Rectangle {
                        color: parent.down ? appWindow.bgActive
                             : (parent.hovered ? appWindow.bgHover : appWindow.bgSubtle)
                        border.color: appWindow.borderSubtle
                        border.width: 1
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        color: appWindow.textMain
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: reviewDialog.close()
                }

                Button {
                    text: reviewDialog.accepted ? "Locked" : "Accept & Lock"
                    enabled: !reviewDialog.accepted
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 120
                    background: Rectangle {
                        color: !parent.enabled ? appWindow.bgSubtle
                             : (parent.down ? appWindow.brandAccentPressed
                             : (parent.hovered ? appWindow.brandAccentHover : appWindow.brandAccent))
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        color: parent.enabled ? "#ffffff" : appWindow.textMuted
                        font.pixelSize: 12
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: extractionClient.acceptReport(reviewDialog.jobId)
                }
            }
        }
    }
}
