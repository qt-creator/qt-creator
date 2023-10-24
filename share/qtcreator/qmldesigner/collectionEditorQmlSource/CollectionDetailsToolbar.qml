// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import Qt.labs.platform as PlatformWidgets
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme
import CollectionEditorBackend

Item {
    id: root

    property real iconHeight: 20
    required property var model
    required property var backend
    property int selectedRow: -1

    implicitHeight: 30
    implicitWidth: leftSideToolbar.width + rightSideToolbar.width

    function addNewColumn() {
        addColumnDialog.popUp(root.model.columnCount())
    }

    function addNewRow() {
        root.model.insertRow(root.model.rowCount())
    }

    Row {
        id: leftSideToolbar

        topPadding: (root.height - root.iconHeight) / 2
        leftPadding: 10
        spacing: 5

        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }

        IconButton {
            icon: StudioTheme.Constants.addcolumnleft_medium
            tooltip: qsTr("Add property left %1").arg(leftSideToolbar.topPadding)
            enabled: root.model.selectedColumn > 0
            onClicked: addColumnDialog.popUp(root.model.selectedColumn - 1)
        }

        IconButton {
            icon: StudioTheme.Constants.addcolumnright_medium
            tooltip: qsTr("Add property right")
            enabled: root.model.selectedColumn > -1
            onClicked: addColumnDialog.popUp(root.model.selectedColumn + 1)
        }

        IconButton {
            icon: StudioTheme.Constants.deletecolumn_medium
            tooltip: qsTr("Delete selected property")
            enabled: root.model.selectedColumn > -1
            onClicked: root.model.removeColumn(root.model.selectedColumn)
        }

        Item { // spacer
            width: 20
            height: 1
        }

        IconButton {
            icon: StudioTheme.Constants.addrowbelow_medium
            tooltip: qsTr("Insert row below")
            enabled: root.model.selectedRow > -1
            onClicked: root.model.insertRow(root.model.selectedRow + 1)
        }

        IconButton {
            icon: StudioTheme.Constants.addrowabove_medium
            tooltip: qsTr("Insert row above")
            enabled: root.model.selectedRow > -1
            onClicked: root.model.insertRow(root.model.selectedRow)
        }

        IconButton {
            icon: StudioTheme.Constants.deleterow_medium
            tooltip: qsTr("Delete selected row")
            enabled: root.model.selectedRow > -1
            onClicked: root.model.removeRow(root.model.selectedRow)
        }
    }

    Row {
        id: rightSideToolbar
        spacing: 5
        topPadding: (root.height - root.iconHeight) / 2
        rightPadding: 10

        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }

        IconButton {
            icon: StudioTheme.Constants.updateContent_medium
            tooltip: qsTr("Update existing file with changes")
            enabled: root.model.collectionName !== ""
            onClicked:
            {
                if (root.backend.selectedSourceAddress().indexOf("json") !== -1)
                    root.model.exportCollection(root.backend.selectedSourceAddress(), root.model.collectionName, "JSON")
                else
                    root.model.exportCollection(root.backend.selectedSourceAddress(), root.model.collectionName, "CSV")
            }
        }

        IconButton {
            icon: StudioTheme.Constants.export_medium
            tooltip: qsTr("Export collection to a new file")
            enabled: root.model.collectionName !== ""
            onClicked: exportMenu.popup()
        }

        StudioControls.Menu {
            id: exportMenu

            StudioControls.MenuItem {
                text: qsTr("Export as JSON")
                onTriggered:
                {
                    fileDialog.defaultSuffix = "json"
                    fileDialog.open()
                }
            }

            StudioControls.MenuItem {
                text: qsTr("Export as CSV")
                onTriggered:
                {
                    fileDialog.defaultSuffix = "csv"
                    fileDialog.open()
                }
            }
        }
    }

    PlatformWidgets.FileDialog {
        id: fileDialog
        fileMode: PlatformWidgets.FileDialog.SaveFile
        onAccepted:
        {
            var fileAddress = file.toString()

            if (fileAddress.indexOf("json") !== -1)
                root.model.exportCollection(fileAddress, root.model.collectionName, "JSON")
            else if (fileAddress.indexOf("csv") !== -1)
                root.model.exportCollection(fileAddress, root.model.collectionName, "CSV")

            fileDialog.reject()
        }
    }

    Rectangle {
        anchors.fill: parent
        z: -1
        color: "grey"
        opacity: 0.6
        radius: 5
    }

    component IconButton: HelperWidgets.IconButton {
        height: root.iconHeight
        width: root.iconHeight
    }

    HelperWidgets.RegExpValidator {
        id: nameValidator
        regExp: /^\w+$/
    }

    StudioControls.Dialog {
        id: addColumnDialog

        property int clickedIndex: -1
        property bool nameIsValid

        title: qsTr("Add Column")
        width: 400

        function popUp(index)
        {
            addColumnDialog.clickedIndex = index
            addColumnDialog.open()
        }

        function addColumnName() {
            if (addColumnDialog.nameIsValid) {
                root.model.addColumn(addColumnDialog.clickedIndex, columnName.text)
                addColumnDialog.accept()
            } else {
                addColumnDialog.reject()
            }
        }

        contentItem: Column {
            spacing: 2

            Row {
                spacing: 10

                Text {
                    text: qsTr("Column name:")
                    color: StudioTheme.Values.themeTextColor
                }

                StudioControls.TextField {
                    id: columnName

                    actionIndicator.visible: false
                    translationIndicator.visible: false
                    validator: nameValidator

                    Keys.onEnterPressed: addColumnDialog.addColumnName()
                    Keys.onReturnPressed: addColumnDialog.addColumnName()
                    Keys.onEscapePressed: addColumnDialog.reject()

                    onTextChanged: {
                        addColumnDialog.nameIsValid = (columnName.text !== ""
                                                       && !root.model.isPropertyAvailable(columnName.text))
                    }
                }
            }

            Text {
                text: qsTr("The collection already contains \"%1\"!").arg(columnName.text)
                visible: columnName.text !== "" && !addColumnDialog.nameIsValid
                color: "red"
            }

            Item { // spacer
                width: 1
                height: 20
            }

            Row {
                anchors.right: parent.right
                spacing: 10

                HelperWidgets.Button {
                    enabled: addColumnDialog.nameIsValid
                    text: qsTr("Add")
                    onClicked: addColumnDialog.addColumnName()
                }

                HelperWidgets.Button {
                    text: qsTr("Cancel")
                    onClicked: addColumnDialog.reject()
                }
            }
        }
    }
}
