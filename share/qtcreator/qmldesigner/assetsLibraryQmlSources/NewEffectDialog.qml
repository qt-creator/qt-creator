// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import AssetsLibraryBackend

StudioControls.Dialog {
    id: root

    title: qsTr("Create New Effect")
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    modal: true

    required property string dirPath
    readonly property int __maxPath: 32

    property var rootView: AssetsLibraryBackend.rootView

    ErrorDialog {
        id: creationFailedDialog
        title: qsTr("Could not create effect")
        message: qsTr("An error occurred while trying to create the effect.")
    }

    contentItem: Column {
        spacing: 2

        Row {
            Text {
                text: qsTr("Effect name: ")
                anchors.verticalCenter: parent.verticalCenter
                color: StudioTheme.Values.themeTextColor
            }

            StudioControls.TextField {
                id: effectName

                actionIndicator.visible: false
                translationIndicator.visible: false

                Keys.onEnterPressed: btnCreate.onClicked()
                Keys.onReturnPressed: btnCreate.onClicked()
                Keys.onEscapePressed: root.reject()

                onTextChanged: {
                    let validator = /^[A-Z]\w{2,}[A-Za-z0-9_]*$/
                    txtNameValidatorMsg.visible = text.length > 0 && !validator.test(text)
                    btnCreate.enabled = !txtNameValidatorMsg.visible
                }
            }
        }

        Text {
            text: qsTr("Effect name cannot be empty.")
            color: "#ff0000"
            anchors.right: parent.right
            visible: effectName.text === ""
        }

        Text {
            text: qsTr("Effect path is too long.")
            color: "#ff0000"
            anchors.right: parent.right
            visible: effectName.text.length > root.__maxPath
        }

        Text {
            id: txtNameValidatorMsg
            text: qsTr('The name must start with a capital letter, ' +
                       'contain at least three characters, ' +
                       'and cannot have any special characters.')
            color: "#ff0000"
            anchors.right: parent.right
            anchors.left: parent.left
            wrapMode: "WordWrap"
        }

        Item { // spacer
            width: 1
            height: 20
        }

        Row {
            anchors.right: parent.right

            HelperWidgets.Button {
                id: btnCreate

                text: qsTr("Create")
                enabled: effectName.text !== ""
                         && effectName.length >=3
                         && effectName.text.length <= root.__maxPath
                onClicked: {
                    const path = AssetsLibraryBackend.rootView.getUniqueEffectPath(root.dirPath, effectName.text)
                    if (AssetsLibraryBackend.rootView.createNewEffect(path))
                        root.accept()
                    else
                        creationFailedDialog.open()
                }
            }

            HelperWidgets.Button {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }

    onOpened: {
        const path = AssetsLibraryBackend.rootView.getUniqueEffectPath(root.dirPath, "Effect01")
        effectName.text = path.split('/').pop().replace(".qep", '')
        effectName.selectAll()
        effectName.forceActiveFocus()
    }
}
