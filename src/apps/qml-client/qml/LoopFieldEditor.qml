import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CarbonEngineClient
import "BRSRMetadata.js" as BrsrMeta

ColumnLayout {
    id: loopField
    spacing: 6
    Layout.fillWidth: true

    property string fieldKey
    property var rows: []
    property bool locked: false
    property bool showJsonView: false

    signal edited(string fieldKey, var rows)

    property var columns: BrsrMeta.getTableColumns(fieldKey, rows)

    function cloneRows() {
        var arr = []
        if (!loopField.rows) return arr
        for (var i = 0; i < loopField.rows.length; ++i) {
            var r = loopField.rows[i]
            var item = {}
            if (typeof r === "object" && r !== null) {
                for (var k in r) item[k] = r[k]
            } else {
                item["value"] = r
            }
            arr.push(item)
        }
        return arr
    }

    function updateCell(rowIdx, colKey, val) {
        if (!loopField.rows || rowIdx < 0 || rowIdx >= loopField.rows.length) return
        var currentObj = loopField.rows[rowIdx]
        if (typeof currentObj === "object" && currentObj !== null) {
            var oldVal = (currentObj[colKey] !== undefined && currentObj[colKey] !== null && currentObj[colKey] !== "null")
                ? String(currentObj[colKey]) : ""
            if (oldVal === String(val)) return
            currentObj[colKey] = val
        }
        var copy = cloneRows()
        if (rowIdx >= 0 && rowIdx < copy.length) {
            copy[rowIdx][colKey] = val
            loopField.edited(loopField.fieldKey, copy)
        }
    }

    function addRow() {
        var copy = cloneRows()
        var cols = loopField.columns
        var newObj = {}
        for (var i = 0; i < cols.length; ++i) {
            newObj[cols[i].key] = ""
        }
        copy.push(newObj)
        loopField.rows = copy
        loopField.edited(loopField.fieldKey, copy)
    }

    function removeRow(rowIdx) {
        var copy = cloneRows()
        if (rowIdx >= 0 && rowIdx < copy.length) {
            copy.splice(rowIdx, 1)
            loopField.rows = copy
            loopField.edited(loopField.fieldKey, copy)
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 34
        color: Theme.bgSubtle
        radius: 4
        border.color: Theme.borderSubtle
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 8

            Label {
                text: BrsrMeta.formatTableTitle(loopField.fieldKey)
                font.bold: true
                font.pixelSize: 12
                color: Theme.textBright
            }

            Rectangle {
                Layout.preferredHeight: 18
                Layout.preferredWidth: countText.implicitWidth + 10
                color: loopField.rows && loopField.rows.length > 0 ? Theme.badgePillBlueBg : Theme.bgHover
                radius: 9
                border.color: loopField.rows && loopField.rows.length > 0 ? Theme.badgePillBlueBorder : Theme.borderSubtle
                border.width: 1

                Text {
                    id: countText
                    anchors.centerIn: parent
                    text: (loopField.rows ? loopField.rows.length : 0) + " record" + ((loopField.rows && loopField.rows.length === 1) ? "" : "s")
                    color: loopField.rows && loopField.rows.length > 0 ? Theme.badgePillBlueText : Theme.textMuted
                    font.pixelSize: 10
                    font.weight: Font.Medium
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                visible: !loopField.locked
                text: "+ Add Row"
                Layout.preferredHeight: 24
                background: Rectangle {
                    color: parent.down ? Theme.successBtnPressed : (parent.hovered ? Theme.successBtnHover : Theme.successBtnBg)
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.pixelSize: 11
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: loopField.addRow()
            }

            Button {
                text: loopField.showJsonView ? "Table View" : "{ } Raw JSON"
                Layout.preferredHeight: 24
                background: Rectangle {
                    color: parent.down ? Theme.bgActive : (parent.hovered ? Theme.bgHover : "transparent")
                    border.color: Theme.borderSubtle
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: Theme.textMuted
                    font.pixelSize: 10
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: loopField.showJsonView = !loopField.showJsonView
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2
        visible: !loopField.showJsonView && loopField.rows && loopField.rows.length > 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: Theme.bgHeader
            border.color: Theme.borderSubtle
            border.width: 1
            radius: 3

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 8

                Repeater {
                    model: loopField.columns
                    delegate: Label {
                        Layout.fillWidth: true
                        Layout.preferredWidth: (modelData.flex || 1) * 100
                        text: modelData.label
                        font.bold: true
                        font.pixelSize: 11
                        color: Theme.textMain
                        elide: Text.ElideRight
                    }
                }

                Label {
                    visible: !loopField.locked
                    Layout.preferredWidth: 26
                    text: ""
                }
            }
        }

        Repeater {
            model: loopField.rows
            delegate: Rectangle {
                id: rowContainer
                readonly property int rowIndex: index
                readonly property var rowItem: modelData
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                color: index % 2 === 0 ? Theme.bgBase : Theme.bgCard
                border.color: Theme.borderSubtle
                border.width: 1
                radius: 3

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 8

                    Repeater {
                        model: loopField.columns
                        delegate: TextField {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: (modelData.flex || 1) * 100
                            Layout.topMargin: 3
                            Layout.bottomMargin: 3
                            enabled: !loopField.locked
                            font.pixelSize: 11
                            color: Theme.textBright
                            selectedTextColor: "#ffffff"
                            selectionColor: Theme.brandAccent
                            selectByMouse: true
                            placeholderText: "—"
                            placeholderTextColor: Theme.textMuted

                            background: Rectangle {
                                color: parent.activeFocus ? Theme.bgActive : Theme.bgInput
                                border.color: parent.activeFocus ? Theme.brandAccent : Theme.borderSubtle
                                border.width: 1
                                radius: 3
                            }

                            text: (rowContainer.rowItem && rowContainer.rowItem[modelData.key] !== undefined && rowContainer.rowItem[modelData.key] !== null && rowContainer.rowItem[modelData.key] !== "null")
                                  ? String(rowContainer.rowItem[modelData.key]) : ""

                            onEditingFinished: {
                                loopField.updateCell(rowContainer.rowIndex, modelData.key, text)
                            }
                        }
                    }

                    Button {
                        visible: !loopField.locked
                        Layout.preferredWidth: 26
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: "Delete this record"
                        background: Rectangle {
                            color: parent.down ? Theme.dangerBtnPressed : (parent.hovered ? Theme.dangerBtnBg : "transparent")
                            radius: 3
                        }
                        contentItem: Text {
                            text: "✕"
                            color: parent.hovered ? "#ffffff" : Theme.textMuted
                            font.pixelSize: 11
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: loopField.removeRow(rowContainer.rowIndex)
                    }
                }
            }
        }
    }

    Rectangle {
        visible: !loopField.showJsonView && (!loopField.rows || loopField.rows.length === 0)
        Layout.fillWidth: true
        Layout.preferredHeight: 48
        color: "transparent"
        border.color: Theme.borderSubtle
        border.width: 1
        radius: 4

        RowLayout {
            anchors.centerIn: parent
            spacing: 12
            Label {
                text: "No records found for this disclosure."
                color: Theme.textMuted
                font.pixelSize: 11
            }
            Button {
                visible: !loopField.locked
                text: "+ Add Entry"
                Layout.preferredHeight: 24
                background: Rectangle {
                    color: parent.down ? Theme.brandAccentPressed : (parent.hovered ? Theme.brandAccentHover : Theme.brandAccent)
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: "#ffffff"
                    font.pixelSize: 11
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: loopField.addRow()
            }
        }
    }

    TextArea {
        visible: loopField.showJsonView
        Layout.fillWidth: true
        Layout.preferredHeight: 120
        font.family: "Consolas, monospace"
        font.pixelSize: 11
        color: Theme.textBright
        wrapMode: TextArea.Wrap
        enabled: !loopField.locked
        background: Rectangle {
            color: Theme.bgBase
            border.color: Theme.borderSubtle
            radius: 3
        }
        text: loopField.showJsonView ? JSON.stringify(loopField.rows, null, 2) : ""
        onEditingFinished: {
            try {
                var parsed = JSON.parse(text)
                if (Array.isArray(parsed)) {
                    loopField.rows = parsed
                    loopField.edited(loopField.fieldKey, parsed)
                }
            } catch (e) {
                if (typeof appWindow !== "undefined" && appWindow) {
                    appWindow.appendStatus("Invalid JSON syntax: " + e)
                }
            }
        }
    }
}
