import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// One loop/table field within a FieldGroupCard: shows a read-only row preview by default,
// with a toggle to edit the raw JSON array directly. Kept as its own component (rather than
// inline in FieldGroupCard's repeater) so there is no parent-chain reasoning involved in
// finding "my own" editing state.
ColumnLayout {
    id: loopField
    spacing: 4

    property string fieldKey
    property var rows: []
    property bool locked: false

    signal edited(string fieldKey, var rows)

    property bool editingJson: false

    RowLayout {
        Layout.fillWidth: true
        Label { text: loopField.fieldKey + " (" + loopField.rows.length + " row(s))"; color: appWindow.textMuted; font.pixelSize: 12 }
        Item { Layout.fillWidth: true }
        Button {
            text: loopField.editingJson ? "Done" : "Edit as JSON"
            enabled: !loopField.locked
            Layout.preferredHeight: 24
            background: Rectangle {
                color: !parent.enabled ? "transparent"
                     : (parent.down ? appWindow.bgActive
                     : (parent.hovered ? appWindow.bgHover : appWindow.bgSubtle))
                border.color: appWindow.borderSubtle
                border.width: 1
                radius: 3
            }
            contentItem: Text {
                text: parent.text
                color: parent.enabled ? appWindow.textMain : appWindow.textMuted
                font.pixelSize: 11
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: loopField.editingJson = !loopField.editingJson
        }
    }

    Repeater {
        visible: !loopField.editingJson
        model: loopField.rows
        delegate: Label {
            Layout.fillWidth: true
            text: JSON.stringify(modelData)
            color: appWindow.textMain
            font.pixelSize: 11
            wrapMode: Text.WordWrap
        }
    }

    TextArea {
        id: jsonEditor
        visible: loopField.editingJson
        Layout.fillWidth: true
        Layout.preferredHeight: 100
        enabled: !loopField.locked
        text: JSON.stringify(loopField.rows, null, 2)
        onEditingFinished: {
            try {
                loopField.edited(loopField.fieldKey, JSON.parse(text))
            } catch (e) {
                appWindow.appendStatus("Invalid JSON for " + loopField.fieldKey + ": " + e)
            }
        }
    }
}
