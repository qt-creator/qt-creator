// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CollectionDetails 1.0 as CollectionDetails
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: root

    required property var model
    required property var backend
    required property var sortedModel

    implicitWidth: 300
    implicitHeight: 400
    color: StudioTheme.Values.themeControlBackground

    function closeDialogs() {
        editPropertyDialog.reject()
        deleteColumnDialog.reject()
        toolbar.closeDialogs()
    }

    MouseArea {
        anchors.fill: parent
        onClicked: tableView.model.deselectAll()
    }

    Column {
        id: topRow
        readonly property real maxAvailableHeight: root.height

        visible: root.model.collectionName !== ""
        width: parent.width
        spacing: 10

        CollectionDetailsToolbar {
            id: toolbar
            model: root.model
            backend: root.backend
            width: parent.width
        }

        GridLayout {
            id: gridLayout
            readonly property real maxAvailableHeight: topRow.maxAvailableHeight
                                                       - topRow.spacing
                                                       - toolbar.height

            columns: 3
            rowSpacing: 1
            columnSpacing: 1
            width: parent.width

            anchors {
                left: parent.left
                leftMargin: StudioTheme.Values.collectionTableHorizontalMargin
            }

            Rectangle {
                id: tableTopLeftCorner

                clip: true
                visible: !tableView.model.isEmpty
                color: StudioTheme.Values.themeControlBackgroundInteraction
                border.color: StudioTheme.Values.themeControlBackgroundInteraction
                border.width: 2

                Layout.preferredWidth: rowIdView.width
                Layout.preferredHeight: headerView.height
                Layout.minimumWidth: rowIdView.width
                Layout.minimumHeight: headerView.height

                Text {
                    anchors.fill: parent
                    font: headerTextMetrics.font
                    text: "#"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: StudioTheme.Values.themeTextColor

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        property int order: Qt.AscendingOrder
                        onClicked: {
                            order = (order == Qt.AscendingOrder) ? Qt.DescendingOrder : Qt.AscendingOrder;
                            tableView.closeEditors()
                            tableView.model.sort(-1, order)
                        }
                    }
                }
            }

            HorizontalHeaderView {
                id: headerView

                property real topPadding: 5
                property real bottomPadding: 5

                Layout.preferredHeight: headerTextMetrics.height + topPadding + bottomPadding
                Layout.columnSpan: 2
                syncView: tableView
                clip: true

                delegate: HeaderDelegate {
                    selectedItem: tableView.model.selectedColumn
                    color: StudioTheme.Values.themeControlBackgroundInteraction

                    MouseArea {
                        id: topHeaderMouseArea

                        anchors.fill: parent
                        anchors.leftMargin: StudioTheme.Values.borderHover
                        anchors.rightMargin: StudioTheme.Values.borderHover
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        hoverEnabled: true
                        onClicked: (mouse) => {
                            tableView.model.selectColumn(index)

                            if (mouse.button === Qt.RightButton) {
                                let posX = index === root.model.columnCount() - 1 ? parent.width - editPropertyDialog.width : 0

                                headerMenu.clickedHeaderIndex = index
                                headerMenu.dialogPos = parent.mapToGlobal(posX, parent.height)
                                headerMenu.popup()
                            } else {
                                headerMenu.close()
                            }
                        }
                    }

                    ToolTip {
                        id: topHeaderToolTip

                        property bool expectedToBeShown: topHeaderMouseArea.containsMouse
                        visible: expectedToBeShown && text !== ""
                        delay: 1000

                        onExpectedToBeShownChanged: {
                            if (expectedToBeShown)
                                text = root.model.propertyType(index)
                        }
                    }
                }

                StudioControls.Menu {
                    id: headerMenu

                    property int clickedHeaderIndex: -1
                    property point dialogPos

                    onClosed: {
                        headerMenu.clickedHeaderIndex = -1
                    }

                    StudioControls.MenuItem {
                        text: qsTr("Edit")
                        onTriggered: editPropertyDialog.openDialog(headerMenu.clickedHeaderIndex,
                                                                   headerMenu.dialogPos)
                    }

                    StudioControls.MenuItem {
                        text: qsTr("Delete")
                        onTriggered: deleteColumnDialog.popUp(headerMenu.clickedHeaderIndex)
                    }

                    StudioControls.MenuItem {
                        text: qsTr("Sort Ascending")
                        onTriggered: {
                            tableView.closeEditors()
                            tableView.model.sort(headerMenu.clickedHeaderIndex, Qt.AscendingOrder)
                        }
                    }

                    StudioControls.MenuItem {
                        text: qsTr("Sort Descending")
                        onTriggered: {
                            tableView.closeEditors()
                            tableView.model.sort(headerMenu.clickedHeaderIndex, Qt.DescendingOrder)
                        }
                    }
                }
            }

            VerticalHeaderView {
                id: rowIdView

                syncView: tableView
                clip: true

                Layout.preferredHeight: tableView.height
                Layout.rowSpan: 2
                Layout.alignment: Qt.AlignTop + Qt.AlignLeft
                width: implicitWidth // suppresses GridLayout warnings when resizing

                delegate: HeaderDelegate {
                    selectedItem: tableView.model.selectedRow
                    color: StudioTheme.Values.themeControlBackgroundHover

                    MouseArea {
                        anchors.fill: parent
                        anchors.topMargin: StudioTheme.Values.borderHover
                        anchors.bottomMargin: StudioTheme.Values.borderHover
                        acceptedButtons: Qt.LeftButton
                        onClicked: tableView.model.selectRow(index)
                    }
                }
            }

            TableView {
                id: tableView

                model: root.sortedModel
                clip: true

                readonly property real maxAvailableHeight: gridLayout.maxAvailableHeight
                                                           - addRowButton.height
                                                           - headerView.height
                                                           - (2 * gridLayout.rowSpacing)
                readonly property real maxAvailableWidth: gridLayout.width
                                                          - StudioTheme.Values.collectionTableHorizontalMargin
                                                          - rowIdView.width
                                                          - addColumnButton.width
                                                          - gridLayout.columnSpacing

                property real childrenWidth: tableView.contentItem.childrenRect.width
                property real childrenHeight: tableView.contentItem.childrenRect.height

                property int targetRow
                property int targetColumn

                property Item popupEditingItem

                Layout.alignment: Qt.AlignTop + Qt.AlignLeft
                Layout.preferredWidth: tableView.contentWidth
                Layout.preferredHeight: tableView.contentHeight
                Layout.minimumWidth: 100
                Layout.minimumHeight: 20
                Layout.maximumWidth: maxAvailableWidth
                Layout.maximumHeight: maxAvailableHeight

                columnWidthProvider: function(column) {
                    if (!isColumnLoaded(column))
                        return -1
                    let w = explicitColumnWidth(column)
                    if (w < 0)
                        w = implicitColumnWidth(column)
                    return Math.max(w, StudioTheme.Values.collectionCellMinimumWidth)
                }

                rowHeightProvider: function(row) {
                    if (!isRowLoaded(row))
                        return -1
                    let h = explicitRowHeight(row)
                    if (h < 0)
                        h = implicitRowHeight(row)
                    return Math.max(h, StudioTheme.Values.collectionCellMinimumHeight)
                }

                function closePopupEditor() {
                    if (tableView.popupEditingItem)
                        tableView.popupEditingItem.closeEditor()
                    tableView.popupEditingItem = null
                }

                function openNewPopupEditor(item, editor) {
                    if (tableView.popupEditingItem !== item) {
                        closePopupEditor()
                        tableView.popupEditingItem = item
                    }
                }

                function closeEditors() {
                    closeEditor()
                    closePopupEditor()
                }

                function ensureRowIsVisible(row) {
                    let rows = tableView.model.rowCount()
                    let rowIsLoaded = tableView.isRowLoaded(row)

                    if (row < 0 || row >= rows || rowIsLoaded) {
                        if (rowIsLoaded)
                            tableView.positionViewAtRow(row, Qt.AlignLeft | Qt.AlignTop)

                        tableView.targetRow = -1
                        return
                    }

                    tableView.targetRow = row
                    tableView.positionViewAtRow(row, Qt.AlignLeft | Qt.AlignTop)
                    ensureTimer.start()
                }

                function ensureColumnIsVisible(column) {
                    let columns = tableView.model.columnCount()
                    let columnIsLoaded = tableView.isColumnLoaded(column)

                    if (column < 0 || column >= columns || columnIsLoaded) {
                        if (columnIsLoaded)
                            tableView.positionViewAtColumn(column, Qt.AlignLeft | Qt.AlignTop)

                        tableView.targetColumn = -1
                        return
                    }

                    tableView.targetColumn = column
                    tableView.positionViewAtColumn(column, Qt.AlignLeft | Qt.AlignTop)
                    ensureTimer.start()
                }

                onMaxAvailableHeightChanged: resetSizeTimer.start()
                onMaxAvailableWidthChanged: resetSizeTimer.start()
                onChildrenWidthChanged: resetSizeTimer.start()
                onChildrenHeightChanged: resetSizeTimer.start()

                delegate: Rectangle {
                    id: itemCell

                    clip: true
                    implicitWidth: 100
                    implicitHeight: StudioTheme.Values.baseHeight
                    color: itemSelected ? StudioTheme.Values.themeControlBackgroundInteraction
                                        : StudioTheme.Values.themeControlBackground
                    border.width: 1
                    border.color: {
                        if (dataTypeWarning !== CollectionDetails.Warning.None)
                            return StudioTheme.Values.themeWarning

                        if (itemSelected)
                            return StudioTheme.Values.themeControlOutlineInteraction

                        return StudioTheme.Values.themeControlBackgroundInteraction
                    }

                    HelperWidgets.ToolTipArea {
                        anchors.fill: parent
                        text: root.model.warningToString(dataTypeWarning)
                        enabled: dataTypeWarning !== CollectionDetails.Warning.None && text !== ""
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton
                        onClicked: (mouse) => {
                            let row = index % tableView.model.rowCount()

                            tableView.model.selectRow(row)
                            cellContextMenu.showMenu(row)
                        }
                    }

                    Loader {
                        id: cellContentLoader

                        property int cellColumnType: columnType ? columnType : 0

                        Component {
                            id: cellText

                            Text {
                                text: display ?? ""
                                color: itemSelected ? StudioTheme.Values.themeInteraction
                                                    : StudioTheme.Values.themeTextColor
                                leftPadding: 5
                                topPadding: 3
                                bottomPadding: 3
                                font.pixelSize: StudioTheme.Values.baseFontSize
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }

                        Component {
                            id: checkBoxComponent

                            StudioControls.CheckBox {
                                id: checkBoxDelegate

                                readonly property bool editValue: edit

                                text: ""
                                actionIndicatorVisible: false
                                checked: checkBoxDelegate.editValue
                                onCheckedChanged: {
                                    if (checkBoxDelegate.editValue !== checkBoxDelegate.checked)
                                        edit = checkBoxDelegate.checked
                                }
                            }
                        }

                        Component {
                            id: colorEditorComponent

                            ColorViewDelegate {
                                id: colorEditorItem

                                readonly property color editValue: edit
                                readonly property color displayValue: display
                                property string _frontColorStr
                                property string _backendColorStr

                                actionIndicatorVisible: false

                                onColorChanged: {
                                    _frontColorStr = colorEditorItem.color.toString()
                                    if (_frontColorStr != _backendColorStr)
                                        edit = colorEditorItem.color
                                }

                                Component.onCompleted: {
                                    colorEditorItem.color = display
                                }

                                onDisplayValueChanged: {
                                    _backendColorStr = colorEditorItem.displayValue.toString()
                                    if (_frontColorStr != _backendColorStr)
                                        colorEditorItem.color = colorEditorItem.displayValue
                                }

                                onEditorOpened: (item, editor) => {
                                    tableView.openNewPopupEditor(item, editor)
                                }
                            }
                        }

                        function resetSource() {
                            if (columnType === CollectionDetails.DataType.Color)
                                cellContentLoader.sourceComponent = colorEditorComponent
                            else if (columnType === CollectionDetails.DataType.Boolean)
                                cellContentLoader.sourceComponent = checkBoxComponent
                            else
                                cellContentLoader.sourceComponent = cellText
                        }

                        Component.onCompleted: resetSource()
                        onCellColumnTypeChanged: resetSource()
                    }

                    TableView.editDelegate: CollectionDetailsEditDelegate {
                        anchors {
                            top: itemCell.top
                            left: itemCell.left
                        }
                        Component.onCompleted: tableView.model.deselectAll()
                    }
                }

                Timer {
                    id: resetSizeTimer

                    interval: 100
                    repeat: false
                    onTriggered: {
                        let cWidth = Math.min(tableView.maxAvailableWidth, tableView.childrenWidth)
                        let cHeight = Math.min(tableView.maxAvailableHeight, tableView.childrenHeight)

                        if (tableView.contentWidth !== cWidth || tableView.contentHeight !== cHeight)
                            tableView.returnToBounds()
                    }
                }

                Timer {
                    id: ensureTimer

                    interval: 100
                    repeat: false
                    onTriggered: {
                        tableView.ensureRowIsVisible(tableView.targetRow)
                        tableView.ensureColumnIsVisible(tableView.targetColumn)
                    }
                }

                Connections {
                    target: tableView.model

                    function onModelReset() {
                        root.closeDialogs()
                        tableView.clearColumnWidths()
                        tableView.clearRowHeights()
                    }

                    function onRowsInserted(parent, first, last) {
                        tableView.closeEditors()
                        tableView.model.selectRow(first)
                        tableView.ensureRowIsVisible(first)
                    }

                    function onColumnsInserted(parent, first, last) {
                        tableView.closeEditors()
                        tableView.model.selectColumn(first)
                        tableView.ensureColumnIsVisible(first)
                    }

                    function onRowsRemoved(parent, first, last) {
                        let nextRow = first - 1
                        if (nextRow < 0 && tableView.model.rowCount(parent) > 0)
                            nextRow = 0

                        tableView.model.selectRow(nextRow)
                    }

                    function onColumnsRemoved(parent, first, last) {
                        let nextColumn = first - 1
                        if (nextColumn < 0 && tableView.model.columnCount(parent) > 0)
                            nextColumn = 0

                        tableView.model.selectColumn(nextColumn)
                    }
                }

                HoverHandler { id: hoverHandler }

                ScrollBar.horizontal: StudioControls.TransientScrollBar {
                    id: horizontalScrollBar
                    style: StudioTheme.Values.viewStyle
                    orientation: Qt.Horizontal

                    show: (hoverHandler.hovered || tableView.focus || horizontalScrollBar.inUse)
                          && horizontalScrollBar.isNeeded
                }

                ScrollBar.vertical: StudioControls.TransientScrollBar {
                    id: verticalScrollBar
                    style: StudioTheme.Values.viewStyle
                    orientation: Qt.Vertical

                    show: (hoverHandler.hovered || tableView.focus || verticalScrollBar.inUse)
                          && verticalScrollBar.isNeeded
                }
            }

            HelperWidgets.IconButton {
                id: addColumnButton

                iconSize:16
                Layout.preferredWidth: 24
                Layout.preferredHeight: tableView.height
                Layout.minimumHeight: 24
                Layout.alignment: Qt.AlignLeft + Qt.AlignVCenter

                icon: StudioTheme.Constants.create_medium
                tooltip: "Add Column"

                onClicked: {
                    tableView.closeEditors()
                    toolbar.addNewColumn()
                }
            }

            HelperWidgets.IconButton {
                id: addRowButton

                iconSize:16
                Layout.preferredWidth: tableView.width
                Layout.preferredHeight: 24
                Layout.minimumWidth: 24
                Layout.alignment: Qt.AlignTop + Qt.AlignHCenter

                icon: StudioTheme.Constants.create_medium
                tooltip: "Add Row"

                onClicked: {
                    tableView.closeEditors()
                    toolbar.addNewRow()
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }

    ColumnLayout {
        id: importsProblem

        visible: !topRow.visible && rootView.dataStoreExists && !rootView.projectImportExists
        width: parent.width
        anchors.verticalCenter: parent.verticalCenter
        clip: true

        Text {
            text: qsTr("Import the project to your design document to make the Model Editor enabled.")
            Layout.alignment: Qt.AlignCenter
            Layout.maximumWidth: parent.width
            leftPadding: StudioTheme.Values.collectionItemTextPadding
            rightPadding: StudioTheme.Values.collectionItemTextPadding
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.mediumFontSize
            wrapMode: Text.Wrap
        }

        HelperWidgets.Button {
            text: qsTr("Enable DataStore (This will add the required import)")
            Layout.alignment: Qt.AlignCenter
            onClicked: rootView.addProjectImport()
            leftPadding: StudioTheme.Values.collectionItemTextPadding
            rightPadding: StudioTheme.Values.collectionItemTextPadding
        }
    }

    Text {
        anchors.centerIn: parent
        text: qsTr("There are no models in this project.\nAdd or import a model.")
        visible: !topRow.visible && !importsProblem.visible
        color: StudioTheme.Values.themeTextColor
        font.pixelSize: StudioTheme.Values.mediumFontSize
    }

    TextMetrics {
        id: headerTextMetrics

        font.pixelSize: StudioTheme.Values.baseFontSize
        text: "Xq"
    }

    StudioControls.Menu {
        id: cellContextMenu

        property int rowIndex: -1

        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        function showMenu(rowIndex) {
            cellContextMenu.rowIndex = rowIndex
            cellContextMenu.popup()
        }

        CellContextMenuItem {
            id: addRowAboveCellMenuItem

            itemText: qsTr("Add row above")
            itemIcon: StudioTheme.Constants.addrowabove_medium
            onTriggered: root.model.insertRow(cellContextMenu.rowIndex)
        }
        CellContextMenuItem {
            id: addRowBelowCellMenuItem

            itemText: qsTr("Add row below")
            itemIcon: StudioTheme.Constants.addrowbelow_medium
            onTriggered: root.model.insertRow(cellContextMenu.rowIndex + 1)
        }
        CellContextMenuItem {
            id: deleteRowCellMenuItem

            itemText: qsTr("Delete row")
            itemIcon: StudioTheme.Constants.deleterow_medium
            onTriggered: root.model.removeRows(cellContextMenu.rowIndex, 1)
        }
    }

    component HeaderDelegate: Rectangle {
        id: headerItem

        required property int selectedItem
        property alias horizontalAlignment: headerText.horizontalAlignment
        property alias verticalAlignment: headerText.verticalAlignment

        implicitWidth: headerText.implicitWidth
        implicitHeight: headerText.implicitHeight
        border.width: 1
        clip: true

        Text {
            id: headerText

            topPadding: headerView.topPadding
            bottomPadding: headerView.bottomPadding
            leftPadding: 5
            rightPadding: 5
            text: display
            font: headerTextMetrics.font
            color: StudioTheme.Values.themeTextColor
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        states: [
            State {
                name: "default"
                when: index !== selectedItem
                PropertyChanges {
                    target: headerItem
                    border.color: StudioTheme.Values.themeControlBackgroundInteraction
                }

                PropertyChanges {
                    target: headerText
                    font.bold: false
                }
            },
            State {
                name: "selected"
                when: index === selectedItem

                PropertyChanges {
                    target: headerItem
                    border.color: StudioTheme.Values.themeControlBackground
                }

                PropertyChanges {
                    target: headerText
                    font.bold: true
                }
            }
        ]
    }

    component CellContextMenuItem: StudioControls.MenuItem {
        id: cellContextMenuItemComponent

        property alias itemText: cellContextMenuText.text
        property alias itemIcon: cellContextMenuIcon.text
        text: ""

        implicitWidth: cellContextMenuRow.width
        implicitHeight: cellContextMenuRow.height

        Row {
            id: cellContextMenuRow

            property color textColor : cellContextMenuItemComponent.enabled
                                       ? cellContextMenuItemComponent.highlighted
                                         ? cellContextMenuItemComponent.style.text.selectedText
                                         : cellContextMenuItemComponent.style.text.idle
                                       : cellContextMenuItemComponent.style.text.disabled

            spacing: 2 * StudioTheme.Values.contextMenuHorizontalPadding
            height: StudioTheme.Values.defaultControlHeight
            leftPadding: StudioTheme.Values.contextMenuHorizontalPadding
            rightPadding: StudioTheme.Values.contextMenuHorizontalPadding

            Text {
                id: cellContextMenuIcon

                color: cellContextMenuRow.textColor
                text: StudioTheme.Constants.addrowabove_medium
                font.family: StudioTheme.Constants.iconFont.family
                font.pixelSize: StudioTheme.Values.myIconFontSize
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                id: cellContextMenuText

                color: cellContextMenuRow.textColor
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    EditPropertyDialog {
        id: editPropertyDialog
        model: root.model
    }

    StudioControls.Dialog {
        id: deleteColumnDialog

        property int clickedIndex: -1

        title: qsTr("Delete Column")
        width: 400

        onAccepted: {
            root.model.removeColumn(clickedIndex)
        }

        function popUp(index)
        {
            deleteColumnDialog.clickedIndex = index
            deleteColumnDialog.open()
        }

        contentItem: ColumnLayout {
            spacing: StudioTheme.Values.sectionColumnSpacing

            Text {
                text: qsTr("Are you sure that you want to delete column \"%1\"?").arg(
                           root.model.headerData(
                               deleteColumnDialog.clickedIndex, Qt.Horizontal))
                color: StudioTheme.Values.themeTextColor
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                spacing: StudioTheme.Values.sectionRowSpacing

                HelperWidgets.Button {
                    text: qsTr("Delete")
                    onClicked: deleteColumnDialog.accept()
                }

                HelperWidgets.Button {
                    text: qsTr("Cancel")
                    onClicked: deleteColumnDialog.reject()
                }
            }
        }
    }
}
