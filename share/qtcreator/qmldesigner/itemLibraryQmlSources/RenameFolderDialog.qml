/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuickDesignerTheme
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Dialog {
    id: renameFolderDialog

    title: qsTr("Rename Folder")
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    implicitWidth: 280
    modal: true

    property bool renameError: false

    contentItem: Column {
        spacing: 2

        StudioControls.TextField {
            id: folderRename

            actionIndicator.visible: false
            translationIndicator.visible: false
            width: renameFolderDialog.width - 12
            validator: folderNameValidator

            onEditChanged: renameFolderDialog.renameError = false
            Keys.onEnterPressed: btnRename.onClicked()
            Keys.onReturnPressed: btnRename.onClicked()
        }

        Text {
            text: qsTr("Folder name cannot be empty.")
            color: "#ff0000"
            visible: folderRename.text === "" && !renameFolderDialog.renameError
        }

        Text {
            text: qsTr("Could not rename folder. Make sure no folder with the same name exists.")
            wrapMode: Text.WordWrap
            width: renameFolderDialog.width - 12
            color: "#ff0000"
            visible: renameFolderDialog.renameError
        }

        Item { // spacer
            width: 1
            height: 10
        }

        Text {
            text: qsTr("If the folder has assets in use, renaming it might cause the project to not work correctly.")
            color: StudioTheme.Values.themeTextColor
            wrapMode: Text.WordWrap
            width: renameFolderDialog.width
            leftPadding: 10
            rightPadding: 10
        }

        Item { // spacer
            width: 1
            height: 20
        }

        Row {
            anchors.right: parent.right

            Button {
                id: btnRename

                text: qsTr("Rename")
                enabled: folderRename.text !== ""
                onClicked: {
                    var success = assetsModel.renameFolder(root.contextDir.dirPath, folderRename.text)
                    if (success)
                        renameFolderDialog.accept()

                    renameFolderDialog.renameError = !success
                }
            }

            Button {
                text: qsTr("Cancel")
                onClicked: renameFolderDialog.reject()
            }
        }
    }

    onOpened: {
        folderRename.text = root.contextDir.dirName
        folderRename.selectAll()
        folderRename.forceActiveFocus()
        renameFolderDialog.renameError = false
    }
}
