// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import AssetsLibraryBackend

StudioControls.Dialog {
    id: root

    title: qsTr("Save Effect")

    closePolicy: Popup.CloseOnEscape
    modal: true
    implicitWidth: 250

    property string compositionName: null

    onOpened: {
        nameText.text = "" //TODO: Generate unique name
        emptyText.opacity = 0
        nameText.forceActiveFocus()
    }

    HelperWidgets.RegExpValidator {
        id: validator
        regExp: /^(\w[^*/><?\\|:]*)$/
    }

    contentItem: Column {
        spacing: 2

        Row {
            id: row
            Text {
                text: qsTr("Effect name: ")
                anchors.verticalCenter: parent.verticalCenter
                color: StudioTheme.Values.themeTextColor
            }

            StudioControls.TextField {
                id: nameText

                actionIndicator.visible: false
                translationIndicator.visible: false
                validator: validator

                onTextChanged: {
                    let validator = /^[A-Z]\w{2,}[A-Za-z0-9_]*$/
                    emptyText.visible = text.length > 0 && !validator.test(text)
                    btnSave.enabled = !emptyText.visible
                }
                Keys.onEnterPressed: btnSave.onClicked()
                Keys.onReturnPressed: btnSave.onClicked()
                Keys.onEscapePressed: root.reject()
            }
        }

        Text {
            id: emptyText

            text: qsTr("Effect name cannot be empty.")
            color: StudioTheme.Values.themeError
            anchors.right: row.right
        }

        Item { // spacer
            width: 1
            height: 20
        }

        Row {
            anchors.right: parent.right

            HelperWidgets.Button {
                id: btnSave

                text: qsTr("Save")
                enabled: nameText.text !== ""
                onClicked: {
                    root.compositionName = nameText.text
                    root.accept() //TODO: Check if name is unique
                }
            }

            HelperWidgets.Button {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }
}
