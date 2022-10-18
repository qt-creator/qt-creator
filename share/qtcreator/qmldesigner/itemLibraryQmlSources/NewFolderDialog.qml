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
    id: newFolderDialog

    title: qsTr("Create New Folder")
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    modal: true

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
            }
        }

        Text {
            text: qsTr("Folder name cannot be empty.")
            color: "#ff0000"
            anchors.right: parent.right
            visible: folderName.text === ""
        }

        Item { // spacer
            width: 1
            height: 20
        }

        Row {
            anchors.right: parent.right

            Button {
                id: btnCreate

                text: qsTr("Create")
                enabled: folderName.text !== ""
                onClicked: {
                    assetsModel.addNewFolder(root.contextDir.dirPath + '/' + folderName.text)
                    newFolderDialog.accept()
                }
            }

            Button {
                text: qsTr("Cancel")
                onClicked: newFolderDialog.reject()
            }
        }
    }

    onOpened: {
        folderName.text = qsTr("New folder")
        folderName.selectAll()
        folderName.forceActiveFocus()
    }
}
