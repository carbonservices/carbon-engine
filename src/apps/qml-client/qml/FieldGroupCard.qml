import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// One field-group's extracted draft: data fields as text inputs, narrative fields as text
// areas, loop/table fields as a read-only preview with a raw-JSON editing fallback. Edits are
// reported upward via fieldEdited so the review screen can persist them through ExtractionClient.
Rectangle {
    id: card

    property var groupData
    property var warnings: []
    property bool locked: false

    signal fieldEdited(string groupId, var data)

    Layout.preferredHeight: contentColumn.implicitHeight + 24
    color: appWindow.bgSidebar
    radius: 4
    border.color: appWindow.borderCol
    border.width: 1

    function currentData() {
        var copy = {}
        var source = groupData.data || {}
        for (var key in source) copy[key] = source[key]
        return copy
    }

    function commitField(key, value) {
        var data = currentData()
        data[key] = value
        groupData.data = data
        fieldEdited(groupData.group_id, data)
    }

    ColumnLayout {
        id: contentColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Label { text: card.groupData.label; font.bold: true; font.pixelSize: 14; color: appWindow.textMain }
            Item { Layout.fillWidth: true }
            Label {
                visible: card.groupData.status === "FAILED"
                text: "Extraction failed"
                color: "#ff6b6b"
                font.pixelSize: 11
            }
        }

        Repeater {
            model: card.warnings
            delegate: Label {
                Layout.fillWidth: true
                text: modelData
                color: "#f0a500"
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }
        }

        Repeater {
            model: card.groupData.data_keys || []
            delegate: RowLayout {
                Layout.fillWidth: true
                Label { text: modelData; Layout.preferredWidth: 220; color: appWindow.textMuted; font.pixelSize: 12 }
                TextField {
                    Layout.fillWidth: true
                    enabled: !card.locked
                    text: (card.groupData.data && card.groupData.data[modelData] !== undefined) ? String(card.groupData.data[modelData]) : ""
                    onEditingFinished: card.commitField(modelData, text)
                }
            }
        }

        Repeater {
            model: card.groupData.narrative_keys || []
            delegate: ColumnLayout {
                Layout.fillWidth: true
                Label { text: modelData; color: appWindow.textMuted; font.pixelSize: 12 }
                TextArea {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 72
                    wrapMode: TextArea.Wrap
                    enabled: !card.locked
                    text: (card.groupData.data && card.groupData.data[modelData] !== undefined) ? String(card.groupData.data[modelData]) : ""
                    onEditingFinished: card.commitField(modelData, text)
                }
            }
        }

        Repeater {
            model: card.groupData.loop_keys || []
            delegate: LoopFieldEditor {
                Layout.fillWidth: true
                fieldKey: modelData
                rows: (card.groupData.data && card.groupData.data[modelData]) ? card.groupData.data[modelData] : []
                locked: card.locked
                onEdited: function(key, newRows) { card.commitField(key, newRows) }
            }
        }
    }
}
