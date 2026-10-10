import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "BrsrMetadata.js" as BrsrMeta

// Professional Tabular Data Grid for BRSR Disclosure Tables & Loop Fields.
// Replaces raw JSON editing with an intuitive spreadsheet-style interface:
// - Human-friendly SEBI disclosure table titles
// - Clear column headers (e.g. "Product Name", "NIC Code", "% of Turnover")
// - In-place editable cells with dark theme styling
// - Add Row / Delete Row operations
// - Collapsible raw JSON view preserved for power users / debugging
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
        var copy = cloneRows()
        if (rowIdx >= 0 && rowIdx < copy.length) {
            copy[rowIdx][colKey] = val
            loopField.rows = copy
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

    // Table Header Bar (Title, Count Badge, Add Row, JSON toggle)
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 34
        color: appWindow.bgSubtle
        radius: 4
        border.color: appWindow.borderSubtle
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
                color: appWindow.textBright
            }

            Rectangle {
                Layout.preferredHeight: 18
                Layout.preferredWidth: countText.implicitWidth + 10
                color: loopField.rows && loopField.rows.length > 0 ? "#1e3a5f" : "#2d2d2d"
                radius: 9
                border.color: loopField.rows && loopField.rows.length > 0 ? "#3b82f6" : "#444444"
                border.width: 1

                Text {
                    id: countText
                    anchors.centerIn: parent
                    text: (loopField.rows ? loopField.rows.length : 0) + " record" + ((loopField.rows && loopField.rows.length === 1) ? "" : "s")
                    color: loopField.rows && loopField.rows.length > 0 ? "#93c5fd" : appWindow.textMuted
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
                    color: parent.down ? "#166534" : (parent.hovered ? "#15803d" : "#16a34a")
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
                    color: parent.down ? appWindow.bgActive : (parent.hovered ? appWindow.bgHover : "transparent")
                    border.color: appWindow.borderSubtle
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: appWindow.textMuted
                    font.pixelSize: 10
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: loopField.showJsonView = !loopField.showJsonView
            }
        }
    }

    // Tabular Grid View
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2
        visible: !loopField.showJsonView && loopField.rows && loopField.rows.length > 0

        // Column Header Row
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: appWindow.bgHeader
            border.color: appWindow.borderSubtle
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
                        color: appWindow.textMain
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

        // Data Rows
        Repeater {
            model: loopField.rows
            delegate: Rectangle {
                id: rowContainer
                readonly property int rowIndex: index
                readonly property var rowItem: modelData
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                color: index % 2 === 0 ? appWindow.bgBase : appWindow.bgCard
                border.color: appWindow.borderSubtle
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
                            color: appWindow.textBright
                            selectedTextColor: "#ffffff"
                            selectionColor: appWindow.brandAccent
                            selectByMouse: true
                            placeholderText: "—"
                            placeholderTextColor: appWindow.textMuted

                            background: Rectangle {
                                color: parent.activeFocus ? appWindow.bgActive : appWindow.bgInput
                                border.color: parent.activeFocus ? appWindow.brandAccent : appWindow.borderSubtle
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

                    // Delete Row Action
                    Button {
                        visible: !loopField.locked
                        Layout.preferredWidth: 26
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        hoverEnabled: true
                        ToolTip.visible: hovered
                        ToolTip.text: "Delete this record"
                        background: Rectangle {
                            color: parent.down ? "#b91c1c" : (parent.hovered ? "#dc2626" : "transparent")
                            radius: 3
                        }
                        contentItem: Text {
                            text: "✕"
                            color: parent.hovered ? "#ffffff" : appWindow.textMuted
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

    // Empty State Box (When 0 rows)
    Rectangle {
        visible: !loopField.showJsonView && (!loopField.rows || loopField.rows.length === 0)
        Layout.fillWidth: true
        Layout.preferredHeight: 48
        color: "transparent"
        border.color: appWindow.borderSubtle
        border.width: 1
        radius: 4

        RowLayout {
            anchors.centerIn: parent
            spacing: 12
            Label {
                text: "No records found for this disclosure."
                color: appWindow.textMuted
                font.pixelSize: 11
            }
            Button {
                visible: !loopField.locked
                text: "+ Add Entry"
                Layout.preferredHeight: 24
                background: Rectangle {
                    color: parent.down ? appWindow.brandAccentPressed : (parent.hovered ? appWindow.brandAccentHover : appWindow.brandAccent)
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

    // Advanced Raw JSON View (when toggled)
    TextArea {
        visible: loopField.showJsonView
        Layout.fillWidth: true
        Layout.preferredHeight: 120
        font.family: "Consolas, monospace"
        font.pixelSize: 11
        color: appWindow.textBright
        wrapMode: TextArea.Wrap
        enabled: !loopField.locked
        background: Rectangle {
            color: appWindow.bgBase
            border.color: appWindow.borderSubtle
            radius: 3
        }
        text: JSON.stringify(loopField.rows, null, 2)
        onEditingFinished: {
            try {
                var parsed = JSON.parse(text)
                if (Array.isArray(parsed)) {
                    loopField.rows = parsed
                    loopField.edited(loopField.fieldKey, parsed)
                }
            } catch (e) {
                appWindow.appendStatus("Invalid JSON syntax: " + e)
            }
        }
    }
}
