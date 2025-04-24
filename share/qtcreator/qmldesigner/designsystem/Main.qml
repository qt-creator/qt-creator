// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T
import Qt.labs.qmlmodels

import QmlDesigner.DesignSystem
import DesignSystemBackend
import DesignSystemControls as DSC

import StudioControls as StudioControls
import StudioTheme as StudioTheme
import StudioQuickUtils

Rectangle {
    id: root

    property string currentCollectionName

    readonly property int cellWidth: 200
    readonly property int cellHeight: 40

    readonly property color backgroundColor: StudioTheme.Values.themePanelBackground
    // TODO This is not a proper color value, but will be fixed with new design
    readonly property color borderColor: StudioTheme.Values.themeControlBackground_topToolbarHover

    readonly property int borderWidth: StudioTheme.Values.border

    readonly property int textSize: 18
    readonly property int iconSize: 16

    readonly property int leftPadding: 14
    readonly property int rightPadding: 14

    property var customStyle: StudioTheme.ControlStyle {
        border.idle: root.borderColor
    }

    width: 400
    height: 400
    color: StudioTheme.Values.themePanelBackground

    function clearModel() {
        root.currentCollectionName = ""
        tableView.model = null

        topLeftCell.visible = false
        createModeButton.enabled = false
        modelConnections.target = null
    }

    function loadModel(name) {
        if (name === undefined) {
            clearModel()
            return
        }

        root.currentCollectionName = name
        tableView.model = DesignSystemBackend.dsInterface.model(name)

        topLeftCell.visible = tableView.model.columnCount()
        createModeButton.enabled = tableView.model.rowCount()
        modelConnections.target = tableView.model
    }

    function groupString(gt) {
        if (gt === GroupType.Strings)
            return "string"
        if (gt === GroupType.Numbers)
            return "number"
        if (gt === GroupType.Colors)
            return "color"
        if (gt === GroupType.Flags)
            return "bool"

        return "unknow_group"
    }

    function setValue(value: var, row: int, column: int, isBinding: bool): bool {
        let result = tableView.model.setData(tableView.index(row, column),
                                       DesignSystemBackend.dsInterface.createThemeProperty("", value, isBinding),
                                       Qt.EditRole)

        if (!result)
            overlayInvalid.showData(row, column)

        return result
    }

    function dismissInvalidOverlay() {
        overlayInvalid.hide()
        notification.visible = false
    }

    Rectangle {
        id: overlayInvalid

        property Item cellItem

        color: "transparent"
        border {
            width: StudioTheme.Values.border
            color: StudioTheme.Values.themeAmberLight
        }

        visible: false
        z: 112

        function show() {
            overlayInvalid.visible = true
            overlayInvalid.layout()
            notification.visible = true

            notification.forceActiveFocus()
        }

        function showData(row: int, column: int) {
            overlayInvalid.parent = tableView.contentItem
            overlayInvalid.cellItem = tableView.itemAtCell(Qt.point(column, row))

            notification.message = qsTr("Invalid binding. Please use a valid non-cyclic binding.")

            overlayInvalid.show()
        }

        function showHeaderData(section: int, orientation: var, message: string) {
            if (orientation === Qt.Horizontal) {
                overlayInvalid.parent = horizontalHeaderView.contentItem
                overlayInvalid.cellItem = horizontalHeaderView.itemAtCell(Qt.point(overlay.section, 0))
            } else {
                overlayInvalid.parent = verticalHeaderView.contentItem
                overlayInvalid.cellItem = verticalHeaderView.itemAtCell(Qt.point(0, overlay.section))
            }

            notification.message = message

            overlayInvalid.show()
        }

        function hide() {
            overlayInvalid.visible = false
        }

        function layout() {
            if (!overlayInvalid.visible)
                return

            if (overlayInvalid.cellItem !== null) {
                overlayInvalid.x = overlayInvalid.cellItem.x + 1
                overlayInvalid.y = overlayInvalid.cellItem.y + 1
                overlayInvalid.width = overlayInvalid.cellItem.width - 2
                overlayInvalid.height = overlayInvalid.cellItem.height - 2
            }
        }

        Connections {
            target: tableView

            function onLayoutChanged() { overlayInvalid.layout() }
        }
    }

    Rectangle {
        id: notification

        property alias message: contentItemText.text

        width: 260
        height: 78
        z: 666

        visible: false

        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20

        color: StudioTheme.Values.themePopoutBackground
        border.color: "#636363"
        border.width: StudioTheme.Values.border

        onActiveFocusChanged: {
            if (!notification.activeFocus)
                root.dismissInvalidOverlay()
        }

        Column {
            id: column
            anchors.fill: parent
            anchors.margins: StudioTheme.Values.border

            Item {
                id: titleBarItem
                width: parent.width
                height: StudioTheme.Values.height

                Row {
                    id: row
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 4
                    spacing: 4

                    Item {
                        id: titleBarContent
                        width: row.width - row.spacing - closeIndicator.width
                        height: row.height

                        Row {
                            anchors.fill: parent
                            spacing: 10

                            T.Label {
                                anchors.verticalCenter: parent.verticalCenter
                                font.pixelSize: root.customStyle.mediumIconFontSize
                                text: StudioTheme.Constants.warning2_medium
                                font.family: StudioTheme.Constants.iconFont.family
                                color: StudioTheme.Values.themeAmberLight
                            }

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: qsTr("Warning")
                                color: StudioTheme.Values.themeTextColor
                            }
                        }
                    }

                    StudioControls.IconIndicator {
                        id: closeIndicator
                        anchors.verticalCenter: parent.verticalCenter
                        icon: StudioTheme.Constants.colorPopupClose
                        pixelSize: StudioTheme.Values.myIconFontSize
                        onClicked: root.dismissInvalidOverlay()
                    }
                }
            }

            Item {
                id: contentItem
                width: parent.width
                height: parent.height - titleBarItem.height

                Column {
                    anchors.fill: parent
                    anchors.margins: 8
                    anchors.topMargin: 4

                    Item {
                        width: parent.width
                        height: parent.height

                        Text {
                            id: contentItemText
                            anchors.fill: parent
                            wrapMode: Text.Wrap
                            elide: Text.ElideRight
                            color: StudioTheme.Values.themeTextColor
                        }
                    }
                }
            }
        }
    }

    Connections {
        id: modelConnections
        ignoreUnknownSignals: true // model might initially be null

        function onModelReset() {
            topLeftCell.visible = tableView.model.columnCount()
            createModeButton.enabled = tableView.model.rowCount()
        }
    }

    StudioControls.Dialog {
        id: renameCollectionDialog
        title: qsTr("Rename collection")
        width: Math.min(300, root.width)
        closePolicy: Popup.CloseOnEscape
        anchors.centerIn: parent
        modal: true

        onOpened: renameCollectionTextField.forceActiveFocus()

        contentItem: Column {
            spacing: 20
            width: parent.width

            StudioControls.TextField {
                id: renameCollectionTextField
                actionIndicatorVisible: false
                translationIndicatorVisible: false
                width: parent.width

                onAccepted: renameCollectionDialog.accept()
                onRejected: renameCollectionDialog.reject()
            }

            Row {
                spacing: 10
                anchors.right: parent.right

                StudioControls.DialogButton {
                    text: qsTr("Rename")
                    enabled: (renameCollectionDialog.previousString !== renameCollectionTextField.text)
                    onClicked: renameCollectionDialog.accept()
                }

                StudioControls.DialogButton {
                    text: qsTr("Cancel")
                    onClicked: renameCollectionDialog.reject()
                }
            }
        }

        onAccepted: {
            DesignSystemBackend.dsInterface.renameCollection(collectionsComboBox.currentText,
                                                             renameCollectionTextField.text)
            renameCollectionDialog.close()
        }

        property string previousString

        onAboutToShow: {
            renameCollectionTextField.text = collectionsComboBox.currentText
            renameCollectionDialog.previousString = collectionsComboBox.currentText
        }
    }

    StudioControls.Dialog {
        id: createCollectionDialog
        property alias newCollectionName: createCollectionTextField.text
        title: qsTr("Create collection")
        width: Math.min(300, root.width)
        closePolicy: Popup.CloseOnEscape
        anchors.centerIn: parent
        modal: true

        onOpened: createCollectionTextField.forceActiveFocus()

        contentItem: Column {
            spacing: 20
            width: parent.width

            StudioControls.TextField {
                id: createCollectionTextField
                actionIndicatorVisible: false
                translationIndicatorVisible: false
                width: parent.width

                onAccepted: createCollectionDialog.accept()
                onRejected: createCollectionDialog.reject()
            }

            Row {
                spacing: 10
                anchors.right: parent.right

                StudioControls.DialogButton {
                    text: qsTr("Create")
                    onClicked: createCollectionDialog.accept()
                }

                StudioControls.DialogButton {
                    text: qsTr("Cancel")
                    onClicked: createCollectionDialog.reject()
                }
            }
        }

        onAccepted: {
            DesignSystemBackend.dsInterface.addCollection(createCollectionTextField.text)
            root.loadModel(createCollectionTextField.text)
            collectionsComboBox.currentIndex = collectionsComboBox.indexOfValue(createCollectionTextField.text)
            createCollectionDialog.close()
        }
    }

    StudioControls.Dialog {
        id: removeCollectionDialog
        title: qsTr("Remove collection")
        width: Math.min(300, root.width)
        closePolicy: Popup.CloseOnEscape
        anchors.centerIn: parent
        modal: true

        onOpened: removeCollectionDialog.forceActiveFocus()

        contentItem: Column {
            spacing: 20
            width: parent.width

            Text {
                id: warningText

                text: qsTr("Are you sure? The action cannot be undone.")
                color: StudioTheme.Values.themeTextColor
                wrapMode: Text.WordWrap
                width: parent.width
            }

            Row {
                spacing: 10
                anchors.right: parent.right

                StudioControls.DialogButton {
                    text: qsTr("Remove")
                    onClicked: removeCollectionDialog.accept()
                }

                StudioControls.DialogButton {
                    text: qsTr("Cancel")
                    onClicked: removeCollectionDialog.reject()
                }
            }
        }

        onAccepted: {
            let currentCollectionName = collectionsComboBox.currentText
            let currentCollectionIndex = collectionsComboBox.currentIndex
            let previousCollectionIndex = (currentCollectionIndex === 0) ? 1 : currentCollectionIndex - 1

            root.loadModel(collectionsComboBox.textAt(previousCollectionIndex))
            DesignSystemBackend.dsInterface.removeCollection(currentCollectionName)
            removeCollectionDialog.close()
        }
    }

    Rectangle {
        id: toolBar

        color: StudioTheme.Values.themeToolbarBackground
        width: root.width
        height: StudioTheme.Values.toolbarHeight

        Row {
            height: StudioTheme.Values.toolbarHeight
            leftPadding: 10 // TODO needs to into values
            spacing: StudioTheme.Values.toolbarSpacing

            StudioControls.ComboBox {
                id: collectionsComboBox
                style: StudioTheme.Values.viewBarControlStyle
                anchors.verticalCenter: parent.verticalCenter
                actionIndicatorVisible: false
                model: DesignSystemBackend.dsInterface.collections
                enabled: collectionsComboBox.count
                onActivated: root.loadModel(collectionsComboBox.currentText)
            }

            StudioControls.ComboBox {
                id: themesComboBox
                style: StudioTheme.Values.viewBarControlStyle
                anchors.verticalCenter: parent.verticalCenter
                actionIndicatorVisible: false
                model: tableView.model?.themeNames ?? null
                enabled: tableView.model
                onActivated: tableView.model.setActiveTheme(themesComboBox.currentText)
            }

            StudioControls.IconTextButton {
                id: moreButton
                anchors.verticalCenter: parent.verticalCenter
                buttonIcon: StudioTheme.Constants.more_medium
                checkable: true
                checked: moreMenu.visible
                tooltip: qsTr("More options")

                onToggled: {
                    if (moreMenu.visible)
                        moreMenu.close()
                    else
                        moreMenu.popup(0, moreButton.height)
                }

                StudioControls.Menu {
                    id: moreMenu

                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

                    StudioControls.MenuItem {
                        text: qsTr("Rename")
                        onTriggered: renameCollectionDialog.open()
                    }

                    StudioControls.MenuItem {
                        text: qsTr("Delete")
                        onTriggered: removeCollectionDialog.open()
                    }

                    StudioControls.MenuSeparator {}

                    StudioControls.MenuItem {
                        text: qsTr("Create collection")
                        onTriggered: {
                            createCollectionDialog.newCollectionName = DesignSystemBackend.dsInterface.generateCollectionName(qsTr("NewCollection"))
                            createCollectionDialog.open()
                        }
                    }
                }
            }

            // TODO this is only for debugging purposes
            Connections {
                target: DesignSystemBackend.dsInterface
                function onCollectionsChanged() {
                    root.loadModel(DesignSystemBackend.dsInterface.collections[0])
                }
            }

            StudioControls.IconTextButton {
                id: refreshButton
                anchors.verticalCenter: parent.verticalCenter
                buttonIcon: StudioTheme.Constants.updateContent_medium
                tooltip: qsTr("Refresh")
                onClicked: DesignSystemBackend.dsInterface.loadDesignSystem()
            }
        }
    }

    component Cell: Rectangle {
        id: cell

        required property var display
        required property int row
        required property int column

        required property bool editing

        required property var resolvedValue
        required property bool isActive
        required property bool isBinding
        required property var propertyValue

        readonly property bool bindingEditor: cell.isBinding || tableView.model.editableOverride
        readonly property bool isValid: cell.resolvedValue !== undefined

        color: root.backgroundColor
        implicitWidth: root.cellWidth
        implicitHeight: root.cellHeight
        border {
            width: root.borderWidth
            color: root.borderColor
        }
    }

    component DataCell: Cell {
        id: dataCell

        HoverHandler { id: cellHoverHandler }

        DSC.BindingIndicator {
            id: bindingIndicator
            icon.text: !dataCell.isValid ? StudioTheme.Constants.warning2_medium
                                         : dataCell.isBinding ? StudioTheme.Constants.actionIconBinding
                                                              : StudioTheme.Constants.actionIcon
            icon.color: !dataCell.isValid ? StudioTheme.Values.themeAmberLight
                                          : dataCell.isBinding ? StudioTheme.Values.themeInteraction
                                                               : StudioTheme.Values.themeTextColor

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 10
            visible: !dataCell.editing && (cellHoverHandler.hovered || dataCell.isBinding)

            onClicked: {
                tableView.closeEditor()
                menu.show(dataCell.row, dataCell.column)
            }

            onHoverChanged: {
                if (dataCell.isValid)
                    return

                if (bindingIndicator.hover)
                    toolTipInvalid.showText(dataCell,
                                            Qt.point(bindingIndicator.x + bindingIndicator.width,
                                                     bindingIndicator.y),
                                            qsTr("Invalid binding. Cyclic binding is not allowed."))
                else
                    toolTipInvalid.hideText()
            }
        }
    }

    StudioControls.ToolTipExt { id: toolTipInvalid }

    DelegateChooser {
        id: chooser
        role: "group"

        DelegateChoice {
            roleValue: GroupType.Strings

            DataCell {
                id: stringDelegate

                Text {
                    anchors.fill: parent
                    leftPadding: root.leftPadding
                    horizontalAlignment: TextInput.AlignLeft
                    verticalAlignment: TextInput.AlignVCenter
                    color: stringDelegate.isBinding ? StudioTheme.Values.themeInteraction
                                                    : StudioTheme.Values.themeTextColor
                    text: stringDelegate.resolvedValue
                    visible: !stringDelegate.editing
                }

                // This edit delegate combines the binding editor and string value editor
                TableView.editDelegate: DSC.TextField {
                    id: stringEditDelegate

                    style: root.customStyle

                    anchors.fill: parent
                    leftPadding: root.leftPadding

                    horizontalAlignment: TextInput.AlignLeft
                    verticalAlignment: TextInput.AlignVCenter

                    text: stringDelegate.isBinding ? stringDelegate.propertyValue
                                                   : tableView.model.editableOverride ? ""
                                                                                      : stringDelegate.resolvedValue

                    Component.onCompleted: stringEditDelegate.selectAll()
                    Component.onDestruction: tableView.model.editableOverride = false

                    TableView.onCommit: {
                        root.setValue(stringEditDelegate.text,
                                      stringDelegate.row,
                                      stringDelegate.column,
                                      stringDelegate.bindingEditor)
                    }
                }
            }
        }

        DelegateChoice {
            roleValue: GroupType.Numbers

            DataCell {
                id: numberDelegate

                Text {
                    anchors.fill: parent
                    leftPadding: root.leftPadding
                    rightPadding: root.rightPadding

                    horizontalAlignment: TextInput.AlignLeft
                    verticalAlignment: TextInput.AlignVCenter

                    color: numberDelegate.isBinding ? StudioTheme.Values.themeInteraction
                                                    : StudioTheme.Values.themeTextColor
                    // -128 is the value of QLocale::FloatingPointShortest
                    text: Number(numberDelegate.resolvedValue).toLocaleString(Utils.locale, 'f', -128)
                }

                // This edit delegate has two different controls, one for number editing and one for
                // binding editing. Depending on the mode one is hidden and the other is shown.
                TableView.editDelegate: FocusScope {
                    id: numberEditDelegateFocusScope
                    anchors.fill: parent

                    property bool alreadyCommited: false

                    DSC.SpinBox {
                        id: numberEditDelegate

                        property real previousValue: 0

                        style: root.customStyle

                        anchors.fill: parent

                        realValue: numberDelegate.resolvedValue
                        realFrom: -1000 // TODO define min/max
                        realTo: 1000
                        editable: true
                        decimals: 2

                        focus: !numberDelegate.bindingEditor
                        visible: !numberDelegate.bindingEditor

                        Component.onCompleted: {
                            numberEditDelegate.previousValue = numberDelegate.resolvedValue
                            numberEditDelegate.contentItem.selectAll()
                        }
                        Component.onDestruction: {
                            if (numberEditDelegateFocusScope.alreadyCommited || numberDelegate.bindingEditor)
                                return

                            let val = numberEditDelegate.realValue

                            if (numberEditDelegate.previousValue === val)
                                return

                            root.setValue(val,
                                          numberDelegate.row,
                                          numberDelegate.column,
                                          numberDelegate.bindingEditor)
                        }
                    }

                    DSC.TextField {
                        id: numberBindingEditDelegate

                        style: root.customStyle

                        anchors.fill: parent
                        leftPadding: root.leftPadding
                        rightPadding: root.rightPadding

                        horizontalAlignment: TextInput.AlignLeft
                        verticalAlignment: TextInput.AlignVCenter

                        text: numberDelegate.isBinding ? numberDelegate.propertyValue : ""

                        focus: numberDelegate.bindingEditor
                        visible: numberDelegate.bindingEditor

                        Component.onCompleted: numberBindingEditDelegate.selectAll()
                        Component.onDestruction: tableView.model.editableOverride = false
                    }

                    TableView.onCommit: {
                        // By default assume binding edit delegate is used
                        let val = numberBindingEditDelegate.text

                        // If binding editor isn't used then the SpinBox value needs to be written
                        if (!numberDelegate.bindingEditor) {
                            numberEditDelegate.valueFromText(numberEditDelegate.contentItem.text,
                                                             numberEditDelegate.locale)
                            // Don't use return value of valueFromText as it is of type int.
                            // Internally the float value is set on realValue property.
                            val = numberEditDelegate.realValue
                        }

                        root.setValue(val,
                                      numberDelegate.row,
                                      numberDelegate.column,
                                      numberDelegate.bindingEditor)

                        numberEditDelegateFocusScope.alreadyCommited = true
                    }
                }
            }
        }

        DelegateChoice {
            roleValue: GroupType.Flags

            DataCell {
                id: flagDelegate

                DSC.Switch {
                    id: flagEditDelegate

                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: root.leftPadding

                    checked: flagDelegate.resolvedValue
                    text: flagDelegate.resolvedValue

                    labelColor: flagDelegate.isBinding ? StudioTheme.Values.themeInteraction
                                                       : StudioTheme.Values.themeTextColor

                    readonly: flagDelegate.isBinding

                    onToggled: {
                        root.setValue(flagEditDelegate.checked,
                                      flagDelegate.row,
                                      flagDelegate.column,
                                      false)
                    }
                }

                // Dummy item to show forbidden cursor when hovering over bound switch control
                Item {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: root.leftPadding

                    width: flagEditDelegate.indicator.width
                    height: flagEditDelegate.indicator.height

                    visible: !flagEditDelegate.enabled

                    HoverHandler {
                        enabled: !flagEditDelegate.enabled
                        cursorShape: Qt.ForbiddenCursor
                    }
                }

                TableView.editDelegate: DSC.TextField {
                    id: flagBindingEditDelegate

                    anchors.fill: parent
                    leftPadding: root.leftPadding

                    horizontalAlignment: TextInput.AlignLeft
                    verticalAlignment: TextInput.AlignVCenter

                    text: flagDelegate.isBinding ? flagDelegate.propertyValue
                                                 : tableView.model.editableOverride ? ""
                                                                                    : flagDelegate.resolvedValue

                    Component.onCompleted: flagBindingEditDelegate.selectAll()
                    Component.onDestruction: tableView.model.editableOverride = false

                    TableView.onCommit: {
                        root.setValue(flagBindingEditDelegate.text,
                                      flagDelegate.row,
                                      flagDelegate.column,
                                      true)
                    }
                }
            }
        }

        DelegateChoice {
            roleValue: GroupType.Colors

            DataCell {
                id: colorDelegate

                Row {
                    id: colorDelegateRow
                    anchors.fill: parent
                    leftPadding: root.leftPadding
                    spacing: 8

                    Rectangle {
                        id: colorDelegatePreview
                        anchors.verticalCenter: parent.verticalCenter

                        width: 20
                        height: 20
                        color: colorDelegate.resolvedValue
                        border.color: "black"
                        border.width: StudioTheme.Values.border

                        Image {
                            anchors.fill: parent
                            source: "qrc:/navigator/icon/checkers.png"
                            fillMode: Image.Tile
                            z: -1
                        }

                        MouseArea {
                            id: colorMouseArea
                            anchors.fill: parent
                            enabled: !colorDelegate.isBinding

                            cursorShape: colorDelegate.isBinding ? Qt.ForbiddenCursor
                                                                 : Qt.PointingHandCursor

                            function togglePopup() {
                                if (colorPopup.visibility) {
                                    colorPopup.close()
                                } else {
                                    colorPopup.modelIndex = tableView.index(colorDelegate.row, colorDelegate.column)

                                    colorPopup.ensureLoader()
                                    colorPopup.show(colorDelegate)

                                    if (loader.status === Loader.Ready) {
                                        loader.item.color = colorDelegate.resolvedValue
                                        loader.item.originalColor = colorDelegate.resolvedValue
                                    }
                                }
                                tableView.closeEditor()
                                colorMouseArea.forceActiveFocus()
                            }

                            onClicked: colorMouseArea.togglePopup()
                        }
                    }

                    Text {
                        height: parent.height
                        verticalAlignment: Qt.AlignVCenter
                        color: colorDelegate.isBinding ? StudioTheme.Values.themeInteraction
                                                       : StudioTheme.Values.themeTextColor
                        text: colorDelegate.resolvedValue
                    }
                }

                // This edit delegate combines the binding editor and hex value editor
                TableView.editDelegate: DSC.TextField {
                    id: colorEditDelegate

                    anchors.fill: parent
                    leftPadding: root.leftPadding
                                 + (colorDelegate.bindingEditor ? 0
                                                                : (colorDelegatePreview.width
                                                                   + colorDelegateRow.spacing))

                    horizontalAlignment: TextInput.AlignLeft
                    verticalAlignment: TextInput.AlignVCenter

                    text: colorDelegate.isBinding ? colorDelegate.propertyValue
                                                  : tableView.model.editableOverride ? ""
                                                                                    : colorDelegate.resolvedValue

                    RegularExpressionValidator {
                        id: hexValidator
                        regularExpression: /#[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?/g
                    }

                    validator: colorDelegate.bindingEditor ? null : hexValidator

                    Component.onCompleted: colorEditDelegate.selectAll()
                    Component.onDestruction: tableView.model.editableOverride = false

                    TableView.onCommit: {
                        root.setValue(colorEditDelegate.text,
                                      colorDelegate.row,
                                      colorDelegate.column,
                                      colorDelegate.bindingEditor)
                    }

                    // Extra color Rectangle to be shown on top of edit delegate
                    Rectangle {
                        anchors.left: parent.left
                        anchors.leftMargin: root.leftPadding
                        anchors.verticalCenter: parent.verticalCenter

                        width: colorDelegatePreview.width
                        height: colorDelegatePreview.height
                        color: colorDelegate.resolvedValue
                        border.color: "black"
                        border.width: StudioTheme.Values.border

                        visible: !colorDelegate.bindingEditor

                        Image {
                            anchors.fill: parent
                            source: "qrc:/navigator/icon/checkers.png"
                            fillMode: Image.Tile
                            z: -1
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: !colorDelegate.isBinding

                            onClicked: colorMouseArea.togglePopup()
                        }
                    }
                }
            }
        }
    }

    StudioControls.Menu {
        id: menu

        property var modelIndex

        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        function show(row, column) {
            menu.modelIndex = tableView.index(row, column)
            menu.popup()
        }

        StudioControls.MenuItem {
            enabled: {
                if (menu.modelIndex)
                    return tableView.itemAtIndex(menu.modelIndex).isBinding

                return false
            }
            text: qsTr("Reset")
            onTriggered: {
                let data = tableView.model.data(menu.modelIndex, CollectionModel.ResolvedValueRole) ?? ""
                var prop = DesignSystemBackend.dsInterface.createThemeProperty("", data, false)
                let result = tableView.model.setData(menu.modelIndex, prop, Qt.EditRole)
            }
        }

        StudioControls.MenuItem {
            text: qsTr("Set Binding")
            onTriggered: {
                let cell = tableView.itemAtIndex(menu.modelIndex)
                tableView.model.editableOverride = true
                tableView.edit(menu.modelIndex)
            }
        }
    }

    StudioControls.Menu {
        id: modeMenu

        property int column

        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        function show(column) { // Qt.Horizontal
            tableView.closeEditor() // Close all currently visible edit delegates
            modeMenu.column = column
            modeMenu.popup()
        }

        StudioControls.MenuItem {
            enabled: false
            text: qsTr("Duplicate mode")
            onTriggered: { console.log("Duplicate mode") }
        }

        StudioControls.MenuItem {
            text: qsTr("Rename mode")
            onTriggered: overlay.show(modeMenu.column, Qt.Horizontal)
        }

        StudioControls.MenuItem {
            text: qsTr("Delete mode")
            onTriggered: tableView.model.removeColumns(modeMenu.column, 1)
        }
    }

    StudioControls.Menu {
        id: variableMenu

        property int row

        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        function show(row) { // Qt.Vertical
            tableView.closeEditor() // Close all currently visible edit delegates
            variableMenu.row = row
            variableMenu.popup()
        }

        StudioControls.MenuItem {
            enabled: false
            text: qsTr("Duplicate variable")
            onTriggered: { console.log("Duplicate variable") }
        }

        StudioControls.MenuItem {
            text: qsTr("Rename variable")
            onTriggered: overlay.show(variableMenu.row, Qt.Vertical)
        }

        StudioControls.MenuItem {
            text: qsTr("Delete variable")
            onTriggered: tableView.model.removeRows(variableMenu.row, 1)
        }
    }

    Rectangle {
        id: overlay

        property int section
        property var orientation
        property var group

        width: 200
        height: 200
        color: "red"
        visible: false

        z: 111
        parent: tableView.contentItem

        DSC.TextField {
            id: overlayTextField

            property string previousText

            anchors.fill: parent
            leftPadding: root.leftPadding + (overlayIcon.visible ? overlayIcon.width + 8 : 0)

            horizontalAlignment: TextInput.AlignLeft
            verticalAlignment: TextInput.AlignVCenter

            onActiveFocusChanged: {
                if (!overlayTextField.activeFocus)
                    overlay.hide()
            }

            onAccepted: {
                let result = tableView.model.setHeaderData(overlay.section,
                                                           overlay.orientation,
                                                           overlayTextField.text,
                                                           Qt.EditRole)

                // Revoke active focus from text field by forcing active focus on another item
                tableView.forceActiveFocus()

                if (overlayTextField.text === "")
                    overlayInvalid.showHeaderData(overlay.section,
                                                  overlay.orientation,
                                                  qsTr("No name found, please enter a valid name."))
                else if (!result && overlayTextField.previousText !== overlayTextField.text)
                    overlayInvalid.showHeaderData(overlay.section,
                                                  overlay.orientation,
                                                  qsTr("This name is already in use, please use a different name."))
            }

            Text {
                id: overlayIcon
                anchors.left: parent.left
                anchors.leftMargin: root.leftPadding

                height: parent.height
                verticalAlignment: Qt.AlignVCenter
                font.family: StudioTheme.Constants.iconFont.family
                font.pixelSize: root.iconSize
                color: StudioTheme.Values.themeTextColor

                visible: overlay.group !== undefined

                text: {
                    if (overlay.group === GroupType.Strings)
                        return StudioTheme.Constants.string_medium

                    if (overlay.group === GroupType.Numbers)
                        return StudioTheme.Constants.number_medium

                    if (overlay.group === GroupType.Flags)
                        return StudioTheme.Constants.flag_medium

                    if (overlay.group === GroupType.Colors)
                        return StudioTheme.Constants.colorSelection_medium

                    return StudioTheme.Constants.error_medium
                }
            }
        }

        function show(section, orientation) {
            tableView.closeEditor() // Close all currently visible edit delegates

            if (orientation === Qt.Horizontal)
                overlay.parent = horizontalHeaderView.contentItem
            else
                overlay.parent = verticalHeaderView.contentItem

            overlay.visible = true
            overlay.section = section
            overlay.orientation = orientation
            overlay.group = tableView.model.headerData(section,
                                                       orientation,
                                                       CollectionModel.GroupRole)

            overlay.layout()

            overlayTextField.text = tableView.model.headerData(section,
                                                               orientation,
                                                               CollectionModel.EditRole)
            overlayTextField.previousText = overlayTextField.text

            overlayTextField.forceActiveFocus()
            overlayTextField.selectAll()
        }

        function hide() {
            overlay.visible = false
        }

        function layout() {
            if (!overlay.visible)
                return

            let item = null

            if (overlay.orientation === Qt.Horizontal)
                item = horizontalHeaderView.itemAtCell(Qt.point(overlay.section, 0))
            else
                item = verticalHeaderView.itemAtCell(Qt.point(0, overlay.section))

            let insideViewport = item !== null

            if (insideViewport) {
                overlay.x = item.x
                overlay.y = item.y
                overlay.width = item.width
                overlay.height = item.height
            }
        }

        Connections {
            target: tableView

            function onLayoutChanged() { overlay.layout() }
        }
    }

    StudioControls.PopupDialog {
        id: colorPopup

        property var modelIndex
        property QtObject loaderItem: loader.item

        keepOpen: loader.item?.eyeDropperActive ?? false

        width: 260

        function ensureLoader() {
            if (!loader.active)
                loader.active = true
        }

        Loader {
            id: loader

            sourceComponent: StudioControls.ColorEditorPopup {
                id: popup
                width: colorPopup.contentWidth
                visible: colorPopup.visible
                parentWindow: colorPopup.window

                onActivateColor: function(color) {
                    popup.color = color
                    var prop = DesignSystemBackend.dsInterface.createThemeProperty("", color, false)
                    let result = tableView.model.setData(colorPopup.modelIndex, prop, Qt.EditRole)
                }
            }

            onLoaded: colorPopup.titleBar = loader.item.titleBarContent
        }
    }

    GridLayout {
        id: mainLayout
        anchors.topMargin: toolBar.height
        anchors.fill: parent

        columnSpacing: 0
        rowSpacing: 0

        columns: 2
        rows: 2

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 6 // TODO correct value
            Layout.leftMargin: 6 // TODO correct value

            HoverHandler { id: hoverHandler }

            // top left cell
            // TODO Can't use Cell as it contains required properties
            Rectangle {
                id: topLeftCell
                anchors.right: horizontalHeaderView.left
                anchors.bottom: verticalHeaderView.top
                anchors.rightMargin: -root.borderWidth
                anchors.bottomMargin: -root.borderWidth

                color: root.backgroundColor
                implicitWidth: verticalHeaderView.width
                implicitHeight: horizontalHeaderView.height
                border {
                    width: root.borderWidth
                    color: root.borderColor
                }

                visible: tableView.model
                z: 99

                Row { // TODO might not be necessary
                    anchors.fill: parent
                    leftPadding: root.leftPadding
                    spacing: 8

                    Text {
                        height: parent.height
                        verticalAlignment: Qt.AlignVCenter
                        color: StudioTheme.Values.themeTextColor
                        text: qsTr("Name")
                    }
                }
            }

            HorizontalHeaderView {
                id: horizontalHeaderView

                anchors.left: scrollView.left
                anchors.top: parent.top

                syncView: tableView
                clip: true

                z: overlay.visible ? 102 : 100

                delegate: Cell {
                    id: horizontalHeaderDelegate

                    TapHandler {
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onTapped: function(eventPoint, button) {
                            if (button === Qt.LeftButton)
                                overlay.show(horizontalHeaderDelegate.column, Qt.Horizontal)

                            if (button === Qt.RightButton)
                                modeMenu.show(horizontalHeaderDelegate.column)
                        }
                    }

                    Row {
                        anchors.fill: parent
                        spacing: 8

                        Text {
                            id: activeIcon
                            height: parent.height
                            leftPadding: root.leftPadding
                            verticalAlignment: Qt.AlignVCenter
                            font.family: StudioTheme.Constants.iconFont.family
                            font.pixelSize: StudioTheme.Values.baseIconFontSize
                            color: StudioTheme.Values.themeTextColor
                            text: StudioTheme.Constants.apply_small
                            visible: horizontalHeaderDelegate.isActive
                        }

                        Text {
                            height: parent.height

                            leftPadding: horizontalHeaderDelegate.isActive ? 0 : root.leftPadding
                            verticalAlignment: Qt.AlignVCenter
                            color: StudioTheme.Values.themeTextColor
                            text: horizontalHeaderDelegate.display
                        }
                    }
                }
            }

            VerticalHeaderView {
                id: verticalHeaderView

                anchors.top: scrollView.top
                anchors.left: parent.left

                syncView: tableView
                clip: true

                z: overlay.visible ? 102 : 100

                delegate: Cell {
                    id: verticalHeaderDelegate

                    required property int group

                    TapHandler {
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onTapped: function(eventPoint, button) {
                            if (button === Qt.LeftButton)
                                overlay.show(verticalHeaderDelegate.row, Qt.Vertical)

                            if (button === Qt.RightButton)
                                variableMenu.show(verticalHeaderDelegate.row)
                        }
                    }

                    Row {
                        anchors.fill: parent
                        leftPadding: root.leftPadding
                        spacing: 8

                        Text {
                            id: icon
                            height: parent.height
                            verticalAlignment: Qt.AlignVCenter
                            font.family: StudioTheme.Constants.iconFont.family
                            font.pixelSize: root.iconSize
                            color: StudioTheme.Values.themeTextColor

                            text: {
                                if (verticalHeaderDelegate.group === GroupType.Strings)
                                    return StudioTheme.Constants.string_medium

                                if (verticalHeaderDelegate.group === GroupType.Numbers)
                                    return StudioTheme.Constants.number_medium

                                if (verticalHeaderDelegate.group === GroupType.Flags)
                                    return StudioTheme.Constants.flag_medium

                                if (verticalHeaderDelegate.group === GroupType.Colors)
                                    return StudioTheme.Constants.colorSelection_medium

                                return StudioTheme.Constants.error_medium
                            }
                        }

                        Text {
                            height: parent.height
                            verticalAlignment: Qt.AlignVCenter
                            color: StudioTheme.Values.themeTextColor
                            text: verticalHeaderDelegate.display
                            visible: !verticalHeaderDelegate.editing
                        }
                    }
                }
            }

            ScrollView {
                id: scrollView
                anchors.left: verticalHeaderView.right
                anchors.top: horizontalHeaderView.bottom
                anchors.right: parent.right
                anchors.bottom: parent.bottom

                anchors.leftMargin: -root.borderWidth
                anchors.topMargin: -root.borderWidth
                anchors.rightMargin: -root.borderWidth

                z: 101

                contentItem: TableView {
                    id: tableView

                    rowHeightProvider: function (col) { return root.cellHeight }
                    columnWidthProvider: function(column) {
                        let w = explicitColumnWidth(column)
                        if (w >= 0)
                            return w

                        return implicitColumnWidth(column)
                    }

                    onModelChanged: {
                        tableView.clearColumnWidths()
                        tableView.contentX = 0
                        tableView.contentY = 0
                    }

                    resizableColumns: true
                    editTriggers: TableView.SingleTapped
                    columnSpacing: -root.borderWidth
                    rowSpacing: -root.borderWidth
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: chooser

                    onActiveFocusChanged: {
                        if (!tableView.activeFocus)
                            tableView.closeEditor()
                    }
                }

                ScrollBar.horizontal: StudioControls.TransientScrollBar {
                    id: horizontalScrollBar
                    style: StudioTheme.Values.viewStyle
                    parent: tableView
                    x: 0
                    y: tableView.height - horizontalScrollBar.height
                    width: tableView.availableWidth - (verticalScrollBar.isNeeded ? verticalScrollBar.thickness : 0)
                    orientation: Qt.Horizontal

                    visible: !tableView.hideHorizontalScrollBar

                    show: (hoverHandler.hovered || tableView.focus || tableView.adsFocus
                           || horizontalScrollBar.inUse || horizontalScrollBar.otherInUse)
                          && horizontalScrollBar.isNeeded
                    otherInUse: verticalScrollBar.inUse
                }

                ScrollBar.vertical: StudioControls.TransientScrollBar {
                    id: verticalScrollBar
                    style: StudioTheme.Values.viewStyle
                    parent: tableView
                    x: tableView.width - verticalScrollBar.width
                    y: 0
                    height: tableView.availableHeight - (horizontalScrollBar.isNeeded ? horizontalScrollBar.thickness : 0)
                    orientation: Qt.Vertical

                    visible: !tableView.hideVerticalScrollBar

                    show: (hoverHandler.hovered || tableView.focus || tableView.adsFocus
                           || horizontalScrollBar.inUse || horizontalScrollBar.otherInUse)
                          && verticalScrollBar.isNeeded
                    otherInUse: horizontalScrollBar.inUse
                }
            }
        }

        Item {
            id: rightBar
            width: StudioTheme.Values.toolbarHeight
            Layout.fillHeight: true

            StudioControls.IconTextButton {
                id: createModeButton
                anchors.centerIn: parent
                buttonIcon: StudioTheme.Constants.add_medium
                text: qsTr("Create mode")
                rotation: -90
                enabled: false

                onClicked: tableView.model.insertColumns(0, 1)
            }
        }

        Item {
            id: bottomBar
            Layout.fillWidth: true
            height: StudioTheme.Values.toolbarHeight

            StudioControls.IconTextButton {
                id: createVariableButton
                anchors.centerIn: parent
                buttonIcon: StudioTheme.Constants.add_medium
                text: qsTr("Create variable")

                checkable: true
                checked: createVariableMenu.visible

                onClicked: {
                    if (createVariableMenu.opened)
                        createVariableMenu.close()
                    else
                        createVariableMenu.popup(0, -createVariableMenu.height)
                }

                StudioControls.Menu {
                    id: createVariableMenu

                    width: createVariableButton.width

                    function insertInitalMode() {
                        if (tableView.model.columnCount())
                            return

                        tableView.model.insertColumn(0)
                    }

                    DSC.MenuItem {
                        text: qsTr("Color")
                        buttonIcon: StudioTheme.Constants.colorSelection_medium
                        onTriggered: {
                            createVariableMenu.insertInitalMode()
                            tableView.model.addProperty(GroupType.Colors,
                                                        "color_new",
                                                        "#800080",
                                                        false)
                        }
                    }

                    DSC.MenuItem {
                        text: qsTr("Number")
                        buttonIcon: StudioTheme.Constants.number_medium
                        onTriggered: {
                            createVariableMenu.insertInitalMode()
                            tableView.model.addProperty(GroupType.Numbers,
                                                        "number_new",
                                                        0,
                                                        false)
                        }
                    }

                    DSC.MenuItem {
                        text: qsTr("String")
                        buttonIcon: StudioTheme.Constants.string_medium
                        onTriggered: {
                            createVariableMenu.insertInitalMode()
                            tableView.model.addProperty(GroupType.Strings,
                                                        "string_new",
                                                        "String value",
                                                        false)
                        }
                    }

                    DSC.MenuItem {
                        text: qsTr("Boolean")
                        buttonIcon: StudioTheme.Constants.flag_medium
                        onTriggered: {
                            createVariableMenu.insertInitalMode()
                            tableView.model.addProperty(GroupType.Flags,
                                                        "boolean_new",
                                                        true,
                                                        false)
                        }
                    }
                }
            }
        }

        Item {
            id: corner
            width: StudioTheme.Values.toolbarHeight
            height: StudioTheme.Values.toolbarHeight
        }
    }
}
