// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Item {
    id: root

    property real iconHeight: 20
    required property var model
    property int selectedRow: -1

    implicitHeight: 30
    implicitWidth: leftSideToolbar.width + rightSideToolbar.width

    signal addPropertyRight()

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
