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

    title: qsTr("Create New Folder")
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    modal: true

    required property string dirPath
    property string createdDirPath: ""
    readonly property int __maxPath: 260

    HelperWidgets.RegExpValidator {
        id: folderNameValidator
        regExp: /^(\w[^*/><?\\|:]*)$/
    }

    ErrorDialog {
        id: creationFailedDialog
        title: qsTr("Could not create folder")
        message: qsTr("An error occurred while trying to create the folder.")
    }

    contentItem: Column {
        spacing: 2

        Row {
            Text {
                text: qsTr("Folder name: ")
                anchors.verticalCenter: parent.verticalCenter
                color: StudioTheme.Values.themeTextColor
            }

            StudioControls.TextField {
                id: folderName

                actionIndicator.visible: false
                translationIndicator.visible: false
                validator: folderNameValidator

                Keys.onEnterPressed: btnCreate.onClicked()
                Keys.onReturnPressed: btnCreate.onClicked()
                Keys.onEscapePressed: root.reject()

                onTextChanged: {
                    root.createdDirPath = root.dirPath + '/' + folderName.text
                }
            }
        }

        Text {
            text: qsTr("Folder name cannot be empty.")
            color: "#ff0000"
            anchors.right: parent.right
            visible: folderName.text === ""
        }

        Text {
            text: qsTr("Folder path is too long.")
            color: "#ff0000"
            anchors.right: parent.right
            visible: root.createdDirPath.length > root.__maxPath
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
                enabled: folderName.text !== "" && root.createdDirPath.length <= root.__maxPath
                onClicked: {
                    let dirPathToCreate = root.dirPath + '/' + folderName.text
                    root.createdDirPath = AssetsLibraryBackend.assetsModel.addNewFolder(root.createdDirPath)

                    if (root.createdDirPath)
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
        folderName.text = qsTr("New folder")
        folderName.selectAll()
        folderName.forceActiveFocus()
    }

    onRejected: {
        root.createdDirPath = ""
    }
}
