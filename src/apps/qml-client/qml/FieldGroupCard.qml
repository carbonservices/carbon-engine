import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CarbonEngineClient
import "BRSRMetadata.js" as BrsrMeta

Rectangle {
    id: card

    property var groupData
    property var warnings: []
    property var issues: []
    readonly property var activeIssues: (issues && issues.length > 0) ? issues : (warnings || [])
    property bool locked: false
    property bool showWarningsDetail: false

    signal fieldEdited(string groupId, var data)

    Layout.preferredHeight: contentColumn.implicitHeight + 28
    color: Theme.bgSidebar
    radius: 6
    border.color: Theme.borderCol
    border.width: 1

    function currentData() {
        var copy = {}
        var source = (groupData && groupData.data) ? groupData.data : {}
        for (var key in source) copy[key] = source[key]
        return copy
    }

    function commitField(key, value) {
        var data = currentData()
        data[key] = value
        groupData.data = data
        fieldEdited(groupData.group_id, data)
    }

    function cleanFieldWarning(rawWarning) {
        var match = rawWarning.match(/field\s*["']([^"']+)["']/i)
        if (match && match[1]) {
            return "\"" + BrsrMeta.formatLabel(match[1]) + "\" was not found in the source document"
        }
        return rawWarning
    }

    function formatIssueText(issueItem) {
        if (!issueItem) return ""
        if (typeof issueItem === "object") {
            var label = issueItem.field_key ? BrsrMeta.formatLabel(issueItem.field_key) : ""
            if (label) {
                return "\"" + label + "\": " + (issueItem.message || "notice")
            }
            return issueItem.message || ""
        }
        return cleanFieldWarning(issueItem)
    }

    ColumnLayout {
        id: contentColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Rectangle {
                Layout.preferredWidth: 4
                Layout.preferredHeight: 18
                color: Theme.brandAccent
                radius: 2
            }

            Label {
                text: (card.groupData && card.groupData.label) ? card.groupData.label : "Disclosure Group"
                font.bold: true
                font.pixelSize: 15
                color: Theme.textBright
            }

            Item { Layout.fillWidth: true }

            Button {
                visible: card.activeIssues && card.activeIssues.length > 0
                Layout.preferredHeight: 24
                background: Rectangle {
                    color: card.showWarningsDetail ? Theme.badgeWarningPillActiveBg : Theme.badgeWarningPillBg
                    border.color: Theme.badgeWarningBorder
                    border.width: 1
                    radius: 12
                }
                contentItem: RowLayout {
                    spacing: 4
                    Text {
                        text: "⚠️ " + card.activeIssues.length + " notice" + (card.activeIssues.length > 1 ? "s" : "")
                        color: Theme.badgeWarningSubtleText
                        font.pixelSize: 11
                        font.weight: Font.Medium
                    }
                    Text {
                        text: card.showWarningsDetail ? "▲" : "▼"
                        color: Theme.badgeWarningSubtleText
                        font.pixelSize: 9
                    }
                }
                onClicked: card.showWarningsDetail = !card.showWarningsDetail
            }

            Label {
                visible: card.groupData && card.groupData.status === "FAILED"
                text: "Extraction Failed"
                color: Theme.dangerColor
                font.pixelSize: 11
                font.bold: true
            }
        }

        Rectangle {
            visible: card.showWarningsDetail && card.activeIssues && card.activeIssues.length > 0
            Layout.fillWidth: true
            Layout.preferredHeight: warningsColumn.implicitHeight + 16
            color: Theme.badgeWarningDarkBg
            border.color: Theme.badgeWarningBorder
            border.width: 1
            radius: 4

            ColumnLayout {
                id: warningsColumn
                anchors.fill: parent
                anchors.margins: 10
                spacing: 4

                Label {
                    text: "Field notices in this section (ESG consultant validation):"
                    font.bold: true
                    font.pixelSize: 11
                    color: Theme.badgeWarningText
                }

                Repeater {
                    model: card.activeIssues
                    delegate: Label {
                        Layout.fillWidth: true
                        text: "• " + card.formatIssueText(modelData)
                        color: (modelData && modelData.severity === "ERROR") ? Theme.dangerColor : Theme.badgeWarningSubtleText
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.borderCol
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width > 750 ? 2 : 1
            columnSpacing: 16
            rowSpacing: 10

            Repeater {
                model: (card.groupData && card.groupData.data_keys) ? card.groupData.data_keys : []
                delegate: ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        Layout.fillWidth: true
                        text: BrsrMeta.formatLabel(modelData)
                        color: Theme.textMain
                        font.pixelSize: 11
                        font.weight: Font.Medium
                        elide: Text.ElideRight
                    }

                    TextField {
                        id: scalarInput
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        enabled: !card.locked
                        font.pixelSize: 12
                        color: Theme.textBright
                        selectedTextColor: "#ffffff"
                        selectionColor: Theme.brandAccent
                        selectByMouse: true
                        placeholderText: "— Not extracted / Click to enter"
                        placeholderTextColor: Theme.textMuted

                        background: Rectangle {
                            color: parent.activeFocus ? Theme.bgActive : Theme.bgInput
                            border.color: parent.activeFocus ? Theme.brandAccent : Theme.borderSubtle
                            border.width: 1
                            radius: 4
                        }

                        text: BrsrMeta.cleanDisplayValue(card.groupData && card.groupData.data ? card.groupData.data[modelData] : undefined)

                        onEditingFinished: {
                            card.commitField(modelData, text)
                        }
                    }
                }
            }
        }

        Repeater {
            model: (card.groupData && card.groupData.narrative_keys) ? card.groupData.narrative_keys : []
            delegate: ColumnLayout {
                Layout.fillWidth: true
                spacing: 6
                Layout.topMargin: 4

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: BrsrMeta.formatLabel(modelData)
                        font.bold: true
                        font.pixelSize: 12
                        color: Theme.textMain
                    }

                    Rectangle {
                        Layout.preferredHeight: 18
                        Layout.preferredWidth: narrativePillText.implicitWidth + 10
                        color: Theme.badgeNarrativeBg
                        border.color: Theme.badgeNarrativeBorder
                        border.width: 1
                        radius: 9

                        Text {
                            id: narrativePillText
                            anchors.centerIn: parent
                            text: "Narrative"
                            color: Theme.badgeNarrativeText
                            font.pixelSize: 10
                        }
                    }
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 90
                    clip: true

                    TextArea {
                        width: parent.width
                        wrapMode: TextArea.Wrap
                        enabled: !card.locked
                        font.pixelSize: 12
                        color: Theme.textBright
                        selectedTextColor: "#ffffff"
                        selectionColor: Theme.brandAccent
                        selectByMouse: true
                        placeholderText: "Enter qualitative disclosure or management response..."
                        placeholderTextColor: Theme.textMuted

                        background: Rectangle {
                            color: parent.activeFocus ? Theme.bgActive : Theme.bgInput
                            border.color: parent.activeFocus ? Theme.brandAccent : Theme.borderSubtle
                            border.width: 1
                            radius: 4
                        }

                        text: BrsrMeta.cleanDisplayValue(card.groupData && card.groupData.data ? card.groupData.data[modelData] : undefined)

                        onEditingFinished: {
                            card.commitField(modelData, text)
                        }
                    }
                }
            }
        }

        Repeater {
            model: (card.groupData && card.groupData.loop_keys) ? card.groupData.loop_keys : []
            delegate: LoopFieldEditor {
                Layout.fillWidth: true
                Layout.topMargin: 6
                fieldKey: modelData
                rows: (card.groupData && card.groupData.data && card.groupData.data[modelData]) ? card.groupData.data[modelData] : []
                locked: card.locked
                onEdited: function(key, newRows) {
                    card.commitField(key, newRows)
                }
            }
        }
    }
}
