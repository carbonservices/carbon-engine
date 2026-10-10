import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: reviewDialog
    title: "Review Extracted Report Data"
    modal: true
    width: Math.min(parent ? parent.width - 80 : 1200, 1200)
    height: Math.min(parent ? parent.height - 80 : 800, 800)
    anchors.centerIn: parent
    standardButtons: Dialog.NoButton
    closePolicy: Popup.NoAutoClose

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
        spacing: 8

        TabBar {
            id: sectionTabBar
            Layout.fillWidth: true
            Repeater {
                model: reviewDialog.sectionOrder
                delegate: TabButton { text: reviewDialog.sectionLabels[modelData] }
            }
        }

        StackLayout {
            id: sectionStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: sectionTabBar.currentIndex

            Repeater {
                model: reviewDialog.sectionOrder
                delegate: ScrollView {
                    clip: true
                    ColumnLayout {
                        width: sectionStack.width
                        spacing: 12

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
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Label {
                Layout.fillWidth: true
                text: reviewDialog.accepted ? "Report accepted and locked." : "Review each section, then accept to lock in the values."
                color: appWindow.textMuted
                font.pixelSize: 11
            }
            Button {
                text: "Close"
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
                onClicked: reviewDialog.close()
            }
            Button {
                text: "Accept & Lock"
                enabled: !reviewDialog.accepted
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
                onClicked: extractionClient.acceptReport(reviewDialog.jobId)
            }
        }
    }
}
