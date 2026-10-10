import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "BrsrMetadata.js" as BrsrMeta

// Modern ESG Field Group Card:
// - Professional SEBI BRSR human-friendly labels for ESG consultants
// - Responsive multi-column form grid for scalar data fields
// - Clean value formatting (preventing "null", "N/A", or [object V4ReferenceObject])
// - Collapsible missing-field warning badge to keep the UI uncluttered
// - Elegant dark-themed inputs and narrative text areas
// - Embedded spreadsheet-style LoopFieldEditor for all tabular disclosures
Rectangle {
    id: card

    property var groupData
    property var warnings: []
    property bool locked: false
    property bool showWarningsDetail: false

    signal fieldEdited(string groupId, var data)

    Layout.preferredHeight: contentColumn.implicitHeight + 28
    color: appWindow.bgSidebar
    radius: 6
    border.color: appWindow.borderCol
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
        // Transform "group_id: field "name" has no extracted value" to clean human text
        var match = rawWarning.match(/field\s*["']([^"']+)["']/i)
        if (match && match[1]) {
            return "\"" + BrsrMeta.formatLabel(match[1]) + "\" was not found in the source document"
        }
        return rawWarning
    }

    ColumnLayout {
        id: contentColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        spacing: 12

        // Card Header Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Rectangle {
                Layout.preferredWidth: 4
                Layout.preferredHeight: 18
                color: appWindow.brandAccent
                radius: 2
            }

            Label {
                text: (card.groupData && card.groupData.label) ? card.groupData.label : "Disclosure Group"
                font.bold: true
                font.pixelSize: 15
                color: appWindow.textBright
            }

            Item { Layout.fillWidth: true }

            // Collapsible missing-metrics pill button
            Button {
                visible: card.warnings && card.warnings.length > 0
                Layout.preferredHeight: 24
                background: Rectangle {
                    color: card.showWarningsDetail ? "#78350f" : "#451a03"
                    border.color: "#b45309"
                    border.width: 1
                    radius: 12
                }
                contentItem: RowLayout {
                    spacing: 4
                    Text {
                        text: "⚠️ " + card.warnings.length + " unpopulated"
                        color: "#fde68a"
                        font.pixelSize: 11
                        font.weight: Font.Medium
                    }
                    Text {
                        text: card.showWarningsDetail ? "▲" : "▼"
                        color: "#fde68a"
                        font.pixelSize: 9
                    }
                }
                onClicked: card.showWarningsDetail = !card.showWarningsDetail
            }

            Label {
                visible: card.groupData && card.groupData.status === "FAILED"
                text: "Extraction Failed"
                color: appWindow.dangerColor
                font.pixelSize: 11
                font.bold: true
            }
        }

        // Expanded Warnings Details (collapsible)
        Rectangle {
            visible: card.showWarningsDetail && card.warnings && card.warnings.length > 0
            Layout.fillWidth: true
            Layout.preferredHeight: warningsColumn.implicitHeight + 16
            color: "#291b00"
            border.color: "#b45309"
            border.width: 1
            radius: 4

            ColumnLayout {
                id: warningsColumn
                anchors.fill: parent
                anchors.margins: 10
                spacing: 4

                Label {
                    text: "Unpopulated fields in this section (ESG consultant notice):"
                    font.bold: true
                    font.pixelSize: 11
                    color: "#fde047"
                }

                Repeater {
                    model: card.warnings
                    delegate: Label {
                        Layout.fillWidth: true
                        text: "• " + card.cleanFieldWarning(modelData)
                        color: "#fef08a"
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        // Section Separator
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: appWindow.borderCol
        }

        // Scalar Data Fields: Modern 2-Column Responsive Form Grid
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
                        color: appWindow.textMain
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
                        color: appWindow.textBright
                        selectedTextColor: "#ffffff"
                        selectionColor: appWindow.brandAccent
                        selectByMouse: true
                        placeholderText: "— Not extracted / Click to enter"
                        placeholderTextColor: appWindow.textMuted

                        background: Rectangle {
                            color: parent.activeFocus ? appWindow.bgActive : appWindow.bgInput
                            border.color: parent.activeFocus ? appWindow.brandAccent : appWindow.borderSubtle
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

        // Narrative Disclosures (Long text fields)
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
                        color: appWindow.textMain
                    }

                    Rectangle {
                        Layout.preferredHeight: 18
                        Layout.preferredWidth: narrativePillText.implicitWidth + 10
                        color: "#1e293b"
                        border.color: "#475569"
                        border.width: 1
                        radius: 9

                        Text {
                            id: narrativePillText
                            anchors.centerIn: parent
                            text: "Narrative"
                            color: "#94a3b8"
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
                        color: appWindow.textBright
                        selectedTextColor: "#ffffff"
                        selectionColor: appWindow.brandAccent
                        selectByMouse: true
                        placeholderText: "Enter qualitative disclosure or management response..."
                        placeholderTextColor: appWindow.textMuted

                        background: Rectangle {
                            color: parent.activeFocus ? appWindow.bgActive : appWindow.bgInput
                            border.color: parent.activeFocus ? appWindow.brandAccent : appWindow.borderSubtle
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

        // Tabular & Loop Fields (Rendered via LoopFieldEditor)
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
