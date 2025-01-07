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

Rectangle {
    id: root

    property string currentCollectionName

    readonly property int cellWidth: 200
    readonly property int cellHeight: 40

    readonly property color textColor: "#ffffff"
    readonly property color iconColor: "#959595"
    readonly property color backgroundColor: "#2c2c2c"
    readonly property color borderColor: "#444444"

    readonly property int borderWidth: 1

    readonly property int textSize: 18
    readonly property int iconSize: 16

    readonly property int leftPadding: 14

    width: 400
    height: 400
    color: StudioTheme.Values.themePanelBackground

    function loadModel(name) {
        root.currentCollectionName = name
        tableView.model = DesignSystemBackend.dsInterface.model(name)
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
                onActivated: root.loadModel(collectionsComboBox.currentText)
            }

            StudioControls.IconTextButton {
                id: moreButton
                anchors.verticalCenter: parent.verticalCenter
                buttonIcon: StudioTheme.Constants.more_medium
                checkable: true
                checked: moreMenu.visible

                onClicked: moreMenu.popup(0, moreButton.height)

                StudioControls.Menu {
                    id: moreMenu

                    StudioControls.MenuItem {
                        text: qsTr("Rename")
                        onTriggered: console.log(">>> Rename collection")
                    }

                    StudioControls.MenuItem {
                        text: qsTr("Delete")
                        onTriggered: console.log(">>> Delete collection")
                    }

                    StudioControls.MenuSeparator {}

                    StudioControls.MenuItem {
                        text: qsTr("Create collection")
                        onTriggered: console.log(">>> Create collection")
                    }
                }
            }

            // TODO this is only for debugging purposes
            Button {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("load")
                onClicked: {
                    DesignSystemBackend.dsInterface.loadDesignSystem()
                    root.loadModel(DesignSystemBackend.dsInterface.collections[0])
                }
            }
        }
    }

    component Cell: Rectangle {
        required property string display
        required property int row
        required property int column

        required property bool editing

        required property bool isBinding

        color: root.backgroundColor
        implicitWidth: root.cellWidth
        implicitHeight: root.cellHeight
        border {
            width: root.borderWidth
            color: root.borderColor
        }
    }

    component DataCell: Cell {
        HoverHandler { id: cellHoverHandler }

        StudioControls.IconIndicator {
            icon: isBinding ? StudioTheme.Constants.actionIconBinding
                            : StudioTheme.Constants.actionIcon
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 10
            visible: cellHoverHandler.hovered

            onClicked: {
                tableView.closeEditor()
            }
        }
    }

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
                    color: StudioTheme.Values.themeTextColor
                    text: stringDelegate.display
                    visible: !stringDelegate.editing
                }

                TableView.editDelegate: DSC.TextField {
                    id: stringEditDelegate

                    anchors.fill: parent
                    leftPadding: root.leftPadding

                    // Only apply more to right padding when hovered
                    //rightPadding

                    horizontalAlignment: TextInput.AlignLeft
                    verticalAlignment: TextInput.AlignVCenter

                    text: stringDelegate.display
                    Component.onCompleted: stringEditDelegate.selectAll()

                    TableView.onCommit: {
                        console.log("onCommit", stringEditDelegate.text)
                        let index = TableView.view.index(stringDelegate.row, stringDelegate.column)
                        var prop = DesignSystemBackend.dsInterface.createThemeProperty("",
                                                                                       stringEditDelegate.text,
                                                                                       stringDelegate.isBinding)
                        TableView.view.model.setData(index, prop, Qt.EditRole)
                    }
                }

                Component.onCompleted: console.log("DelegateChoice - string", stringDelegate.display)
            }
        }

        DelegateChoice {
            roleValue: GroupType.Numbers

            DataCell {
                id: numberDelegate

                Text {
                    anchors.fill: parent
                    leftPadding: root.leftPadding
                    horizontalAlignment: TextInput.AlignLeft
                    verticalAlignment: TextInput.AlignVCenter

                    color: StudioTheme.Values.themeTextColor
                    text: numberDelegate.display
                    visible: !numberDelegate.editing
                }

                TableView.editDelegate: SpinBox {
                    id: numberEditDelegate

                    anchors.fill: parent
                    leftPadding: root.leftPadding

                    value: parseInt(numberDelegate.display)
                    from: -1000 // TODO define min/max
                    to: 1000
                    editable: true

                    TableView.onCommit: {
                        let val = numberEditDelegate.valueFromText(numberEditDelegate.contentItem.text,
                                                                   numberEditDelegate.locale)
                        console.log("onCommit", val)
                        let index = TableView.view.index(numberDelegate.row, numberDelegate.column)
                        var prop = DesignSystemBackend.dsInterface.createThemeProperty("",
                                                                                       val,
                                                                                       numberDelegate.isBinding)
                        TableView.view.model.setData(index, prop, Qt.EditRole)
                    }
                }

                Component.onCompleted: console.log("DelegateChoice - number", display)
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

                    checked: flagDelegate.display === "true"
                    text: flagDelegate.display

                    onToggled: {
                        console.log("onCommit", flagEditDelegate.checked)
                        let index = flagDelegate.TableView.view.index(flagDelegate.row, flagDelegate.column)
                        var prop = DesignSystemBackend.dsInterface.createThemeProperty("",
                                                                                       flagEditDelegate.checked,
                                                                                       flagEditDelegate.isBinding)
                        flagDelegate.TableView.view.model.setData(index, prop, Qt.EditRole)
                    }
                }

                Component.onCompleted: console.log("DelegateChoice - bool", flagDelegate.display)
            }
        }

        DelegateChoice {
            roleValue: GroupType.Colors

            DataCell {
                id: colorDelegate

                Row {
                    anchors.fill: parent
                    leftPadding: root.leftPadding
                    spacing: 8

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter

                        width: 20
                        height: 20
                        color: colorDelegate.display
                        border.color: "black"
                        border.width: 1

                        Image {
                            anchors.fill: parent
                            source: "qrc:/navigator/icon/checkers.png"
                            fillMode: Image.Tile
                            z: -1
                        }

                        MouseArea {
                            id: colorMouseArea
                            anchors.fill: parent

                            onClicked: {
                                if (popupDialog.visibility) {
                                    popupDialog.close()
                                } else {
                                    popupDialog.ensureLoader()
                                    popupDialog.show(colorDelegate)

                                    if (loader.status === Loader.Ready)
                                        loader.item.originalColor = root.color
                                }
                                colorMouseArea.forceActiveFocus()
                            }
                        }
                    }

                    Text {
                        height: parent.height
                        verticalAlignment: Qt.AlignVCenter
                        color: StudioTheme.Values.themeTextColor
                        text: colorDelegate.display
                    }
                }

                Component.onCompleted: console.log("DelegateChoice - color", colorDelegate.display)
            }
        }
    }

    StudioControls.PopupDialog {
        id: popupDialog

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
                width: popupDialog.contentWidth
                visible: popupDialog.visible
                parentWindow: popupDialog.window

                onActivateColor: function(color) {
                    console.log("set color", color)
                    //colorBackend.activateColor(color)
                }
            }

            Binding {
                target: loader.item
                property: "color"
                value: root.color
                when: loader.status === Loader.Ready
            }

            onLoaded: {
                loader.item.originalColor = root.color
                popupDialog.titleBar = loader.item.titleBarContent
            }
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
                anchors.right: horizontalHeader.left
                anchors.bottom: verticalHeader.top
                anchors.rightMargin: -root.borderWidth
                anchors.bottomMargin: -root.borderWidth

                color: root.backgroundColor
                implicitWidth: verticalHeader.width
                implicitHeight: horizontalHeader.height
                border {
                    width: root.borderWidth
                    color: root.borderColor
                }

                visible: tableView.model // TODO good enough?
                z: 101

                Row { // TODO might not be necessary
                    anchors.fill: parent
                    leftPadding: root.leftPadding
                    spacing: 8

                    Text {
                        height: parent.height
                        verticalAlignment: Qt.AlignVCenter
                        color: StudioTheme.Values.themeTextColor
                        text: "Name"
                    }
                }
            }

            HorizontalHeaderView {
                id: horizontalHeader

                anchors.left: scrollView.left
                anchors.top: parent.top

                syncView: tableView
                clip: true

                z: 100

                delegate: Cell {
                    id: horizontalHeaderDelegate

                    property bool customEditing: false

                    TapHandler {
                        onTapped: {
                            console.log("onTapped")
                            tableView.closeEditor()
                            horizontalHeaderDelegate.customEditing = true
                            horizontalHeaderTextField.forceActiveFocus()
                        }
                    }

                    Text {
                        anchors.fill: parent
                        leftPadding: root.leftPadding
                        verticalAlignment: Qt.AlignVCenter
                        color: StudioTheme.Values.themeTextColor
                        text: horizontalHeaderDelegate.display
                        visible: !horizontalHeaderDelegate.customEditing
                    }

                    DSC.TextField {
                        id: horizontalHeaderTextField

                        anchors.fill: parent
                        leftPadding: root.leftPadding
                        horizontalAlignment: TextInput.AlignLeft
                        verticalAlignment: TextInput.AlignVCenter
                        text: horizontalHeaderDelegate.display

                        visible: horizontalHeaderDelegate.customEditing

                        //Component.onCompleted: stringEditDelegate.selectAll()

                        onActiveFocusChanged: {
                            if (!horizontalHeaderTextField.activeFocus)
                                horizontalHeaderDelegate.customEditing = false
                        }

                        onEditingFinished: {
                            console.log("onEditingFinished", horizontalHeaderTextField.text)
                            horizontalHeaderDelegate.TableView.view.model.setHeaderData(horizontalHeaderDelegate.column,
                                                                                        Qt.Horizontal,
                                                                                        horizontalHeaderTextField.text,
                                                                                        Qt.EditRole)
                        }
                    }
                }
            }

            VerticalHeaderView {
                id: verticalHeader

                anchors.top: scrollView.top
                anchors.left: parent.left

                syncView: tableView
                clip: true

                z: 100

                delegate: Cell {
                    id: verticalHeaderDelegate

                    required property int group

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
                anchors.left: verticalHeader.right
                anchors.top: horizontalHeader.bottom
                anchors.right: parent.right
                anchors.bottom: parent.bottom

                anchors.leftMargin: -root.borderWidth
                anchors.topMargin: -root.borderWidth
                anchors.rightMargin: -root.borderWidth

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
                        console.log("onModelChanged")
                        tableView.clearColumnWidths()
                        tableView.contentX = 0
                        tableView.contentY = 0
                    }

                    resizableColumns: true
                    editTriggers: TableView.SingleTapped
                    columnSpacing: -root.borderWidth
                    rowSpacing: -root.borderWidth
                    clip: true

                    delegate: chooser
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
                anchors.centerIn: parent
                buttonIcon: StudioTheme.Constants.add_medium
                text: qsTr("Create mode")
                rotation: -90

                onClicked: {
                    tableView.model.insertColumn(0)
                }
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

                    DSC.MenuItem {
                        text: qsTr("Color")
                        buttonIcon: StudioTheme.Constants.colorSelection_medium
                        onTriggered: {
                            console.log(">>> Add Color Property")
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
                            console.log(">>> Add Number Property")
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
                            console.log(">>> Add String Property")
                            tableView.model.addProperty(GroupType.Flags,
                                                         "string_new",
                                                         "String value",
                                                         false)
                        }
                    }

                    DSC.MenuItem {
                        text: qsTr("Boolean")
                        buttonIcon: StudioTheme.Constants.flag_medium
                        onTriggered: {
                            console.log(">>> Add Boolean Property")
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
