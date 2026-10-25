import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CarbonEngineClient
import "BRSRMetadata.js" as BrsrMeta

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
        color: Theme.bgBase
        border.color: Theme.borderDialog
        border.width: 1
        radius: 6
    }

    property int jobId: -1
    property var sectionsByGroupId: ({})
    property var warningsByGroupId: ({})
    property var issuesByGroupId: ({})
    property bool accepted: false

    signal reportAccepted(int jobId)

    property var sectionOrder: ["A", "B", "P1", "P2", "P3", "P4", "P5", "P6", "P7", "P8", "P9", "CORE"]
    property var sectionLabels: ({
        "A": "Section A", "B": "Section B", "P1": "Principle 1", "P2": "Principle 2",
        "P3": "Principle 3", "P4": "Principle 4", "P5": "Principle 5", "P6": "Principle 6",
        "P7": "Principle 7", "P8": "Principle 8", "P9": "Principle 9", "CORE": "BRSR Core"
    })

    ExtractionClient {
        id: extractionClient
        onSchemaReady: function(schema) {
            BrsrMeta.applyServerSchema(schema)
            if (schema && schema.section_labels) {
                reviewDialog.sectionLabels = schema.section_labels
            }
            if (schema && schema.section_order && schema.section_order.length > 0) {
                reviewDialog.sectionOrder = schema.section_order
            }
        }
        onSectionsReady: function(sections, warnings, issues) {
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

            var issuesByGroup = {}
            if (issues && issues.length) {
                for (var j = 0; j < issues.length; ++j) {
                    var issue = issues[j]
                    if (!issuesByGroup[issue.group_id]) issuesByGroup[issue.group_id] = []
                    issuesByGroup[issue.group_id].push(issue)
                }
            }
            reviewDialog.issuesByGroupId = issuesByGroup
        }
        onUpdateSucceeded: function(groupId) {
            if (typeof appWindow !== "undefined" && appWindow) appWindow.appendStatus("Saved edits for " + groupId)
        }
        onAcceptSucceeded: function() {
            reviewDialog.accepted = true
            reviewDialog.reportAccepted(reviewDialog.jobId)
            reviewDialog.close()
        }
        onRequestFailed: function(reason) {
            if (typeof appWindow !== "undefined" && appWindow) appWindow.appendStatus("Review action failed: " + reason)
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
        extractionClient.fetchSchema()
        extractionClient.fetchSections(id)
        open()
    }

    onOpened: sectionTabBar.currentIndex = 0

    contentItem: ColumnLayout {
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: Theme.bgHeader
            border.color: Theme.borderCol
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
                        color: Theme.textBright
                    }
                    Label {
                        text: "Verify and calibrate ESG metrics and disclosure tables before final report generation and audit lock."
                        font.pixelSize: 11
                        color: Theme.textMuted
                    }
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    Layout.preferredHeight: 22
                    Layout.preferredWidth: jobPillText.implicitWidth + 14
                    color: Theme.badgePillBlueBg
                    radius: 11
                    border.color: Theme.badgePillBlueBorder
                    border.width: 1

                    Text {
                        id: jobPillText
                        anchors.centerIn: parent
                        text: "Job #" + reviewDialog.jobId
                        color: Theme.badgePillBlueText
                        font.pixelSize: 11
                        font.bold: true
                    }
                }

                Button {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    background: Rectangle {
                        color: parent.down ? Theme.bgActive : (parent.hovered ? Theme.bgHover : "transparent")
                        radius: 4
                    }
                    contentItem: Text {
                        text: "✕"
                        color: Theme.textMuted
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: reviewDialog.close()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            color: Theme.bgSidebar
            border.color: Theme.borderCol
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
                            color: tabBtn.checked ? Theme.bgBase : (tabBtn.hovered ? Theme.bgHover : "transparent")
                            Rectangle {
                                anchors.bottom: parent.bottom
                                anchors.left: parent.left
                                anchors.right: parent.right
                                height: 2
                                color: tabBtn.checked ? Theme.brandAccent : "transparent"
                            }
                        }

                        contentItem: Text {
                            text: tabBtn.text
                            color: tabBtn.checked ? Theme.textBright : (tabBtn.hovered ? Theme.textMain : Theme.textMuted)
                            font.pixelSize: 11
                            font.bold: tabBtn.checked
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }
        }

        ScrollView {
            id: sectionScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 12
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ColumnLayout {
                width: sectionScrollView.width - 24
                spacing: 14

                Repeater {
                    model: reviewDialog.groupsForSection(reviewDialog.sectionOrder[sectionTabBar.currentIndex])
                    delegate: FieldGroupCard {
                        Layout.fillWidth: true
                        groupData: modelData
                        issues: reviewDialog.issuesByGroupId[modelData.group_id] || []
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

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            color: Theme.bgHeader
            border.color: Theme.borderCol
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
                    color: reviewDialog.accepted ? Theme.successColor : Theme.textMuted
                    font.pixelSize: 11
                }

                Button {
                    text: "Close"
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 80
                    background: Rectangle {
                        color: parent.down ? Theme.bgActive
                             : (parent.hovered ? Theme.bgHover : Theme.bgSubtle)
                        border.color: Theme.borderSubtle
                        border.width: 1
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        color: Theme.textMain
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
                        color: !parent.enabled ? Theme.bgSubtle
                             : (parent.down ? Theme.brandAccentPressed
                             : (parent.hovered ? Theme.brandAccentHover : Theme.brandAccent))
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        color: parent.enabled ? "#ffffff" : Theme.textMuted
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
