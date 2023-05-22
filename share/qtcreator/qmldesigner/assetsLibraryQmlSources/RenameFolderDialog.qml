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

    title: qsTr("Rename Folder")
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    implicitWidth: 280
    modal: true

    property bool renameError: false
    property string renamedDirPath: ""
    required property string dirPath
    required property string dirName

    HelperWidgets.RegExpValidator {
        id: folderNameValidator
        regExp: /^(\w[^*/><?\\|:]*)$/
    }

    contentItem: Column {
        spacing: 2

        StudioControls.TextField {
            id: folderRename

            actionIndicator.visible: false
            translationIndicator.visible: false
            width: root.width - 12
            validator: folderNameValidator

            onEditChanged: root.renameError = false
            Keys.onEnterPressed: btnRename.onClicked()
            Keys.onReturnPressed: btnRename.onClicked()
        }

        Text {
            text: qsTr("Folder name cannot be empty.")
            color: "#ff0000"
            visible: folderRename.text === "" && !root.renameError
        }

        Text {
            text: qsTr("Could not rename folder. Make sure no folder with the same name exists.")
            wrapMode: Text.WordWrap
            width: root.width - 12
            color: "#ff0000"
            visible: root.renameError
        }

        Item { // spacer
            width: 1
            height: 10
        }

        Text {
            text: qsTr("If the folder has assets in use, renaming it might cause the project to not work correctly.")
            color: StudioTheme.Values.themeTextColor
            wrapMode: Text.WordWrap
            width: root.width
            leftPadding: 10
            rightPadding: 10
        }

        Item { // spacer
            width: 1
            height: 20
        }

        Row {
            anchors.right: parent.right

            HelperWidgets.Button {
                id: btnRename

                text: qsTr("Rename")
                enabled: folderRename.text !== ""
                onClicked: {
                    var success = AssetsLibraryBackend.assetsModel.renameFolder(root.dirPath, folderRename.text)
                    if (success) {
                        root.renamedDirPath = root.dirPath.replace(/(.*\/)[^/]+$/, "$1" + folderRename.text)
                        root.accept()
                    }

                    root.renameError = !success
                }
            }

            HelperWidgets.Button {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }

    onOpened: {
        folderRename.text = root.dirName
        folderRename.selectAll()
        folderRename.forceActiveFocus()
        root.renameError = false
    }

    onRejected: {
        root.renamedDirPath = ""
    }
}
