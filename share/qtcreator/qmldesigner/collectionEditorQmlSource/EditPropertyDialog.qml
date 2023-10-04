// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioTheme 1.0 as StudioTheme
import StudioControls 1.0 as StudioControls
import HelperWidgets 2.0 as HelperWidgets

StudioControls.Dialog {
    id: root

    required property var model
    property int __propertyIndex: -1

    title: qsTr("Edit Property")

    function editProperty(index) {
        root.__propertyIndex = index

        if (root.__propertyIndex < 0)
            return

        let previousName = root.model.headerData(root.__propertyIndex, Qt.Horizontal)

        oldName.text = previousName
        newNameField.text = previousName

        root.open()
    }

    onAccepted: {
        if (newNameField.text !== "" && newNameField.text !== oldName.text)
            root.model.renameColumn(root.__propertyIndex, newNameField.text)
    }

    contentItem: Column {
        spacing: 2

        Grid {
            spacing: 10
            columns: 2

            Text {
                text: qsTr("Previous name")
                color: StudioTheme.Values.themeTextColor
            }

            Text {
                id: oldName
                color: StudioTheme.Values.themeTextColor
            }

            Text {
                text: qsTr("New name")
                color: StudioTheme.Values.themeTextColor
            }

            StudioControls.TextField {
                id: newNameField

                actionIndicator.visible: false
                translationIndicator.visible: false

                Keys.onEnterPressed: root.accept()
                Keys.onReturnPressed: root.accept()
                Keys.onEscapePressed: root.reject()

                validator: HelperWidgets.RegExpValidator {
                    regExp: /^\w+$/
                }

                onTextChanged: {
                    editButton.enabled = newNameField.text !== ""
                }
            }
        }

        Item { // spacer
            width: 1
            height: 20
        }

        Row {
            anchors.right: parent.right
            spacing: 10

            HelperWidgets.Button {
                id: editButton

                text: qsTr("Edit")
                onClicked: root.accept()
            }

            HelperWidgets.Button {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }
}
