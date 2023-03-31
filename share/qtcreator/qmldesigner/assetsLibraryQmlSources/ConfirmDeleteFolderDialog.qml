// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme
import StudioControls as StudioControls
import AssetsLibraryBackend

StudioControls.Dialog {
    id: root

    title: qsTr("Folder Not Empty")
    anchors.centerIn: parent
    closePolicy: Popup.CloseOnEscape
    implicitWidth: 300
    modal: true

    required property string dirName
    required property var dirIndex

    contentItem: Column {
        spacing: 20
        width: parent.width

        Text {
            id: folderNotEmpty

            text: qsTr("Folder \"%1\" is not empty. Delete it anyway?").arg(root.dirName)
            color: StudioTheme.Values.themeTextColor
            wrapMode: Text.WordWrap
            width: root.width
            leftPadding: 10
            rightPadding: 10

            Keys.onEnterPressed: btnDelete.onClicked()
            Keys.onReturnPressed: btnDelete.onClicked()
        }

        Text {
            text: qsTr("If the folder has assets in use, deleting it might cause the project to not work correctly.")
            color: StudioTheme.Values.themeTextColor
            wrapMode: Text.WordWrap
            width: root.width
            leftPadding: 10
            rightPadding: 10
        }

        Row {
            anchors.right: parent.right
            HelperWidgets.Button {
                id: btnDelete

                text: qsTr("Delete")

                onClicked: {
                    AssetsLibraryBackend.assetsModel.deleteFolderRecursively(root.dirIndex)
                    root.accept()
                }
            }

            HelperWidgets.Button {
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }

    onOpened: folderNotEmpty.forceActiveFocus()
}
