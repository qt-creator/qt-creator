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
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

StudioControls.Menu {
    id: contextMenu

    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

    onOpened: {
        var numSelected = Object.values(root.selectedAssets).filter(p => p).length
        deleteFileItem.text = numSelected > 1 ? qsTr("Delete Files") : qsTr("Delete File")
    }

    StudioControls.MenuItem {
        text: qsTr("Expand All")
        enabled: root.allExpandedState !== 1
        visible: root.isDirContextMenu
        height: visible ? implicitHeight : 0
        onTriggered: assetsModel.toggleExpandAll(true)
    }

    StudioControls.MenuItem {
        text: qsTr("Collapse All")
        enabled: root.allExpandedState !== 2
        visible: root.isDirContextMenu
        height: visible ? implicitHeight : 0
        onTriggered: assetsModel.toggleExpandAll(false)
    }

    StudioControls.MenuSeparator {
        visible: root.isDirContextMenu
        height: visible ? StudioTheme.Values.border : 0
    }

    StudioControls.MenuItem {
        id: deleteFileItem
        text: qsTr("Delete File")
        visible: root.contextFilePath
        height: deleteFileItem.visible ? deleteFileItem.implicitHeight : 0
        onTriggered: {
            assetsModel.deleteFiles(Object.keys(root.selectedAssets).filter(p => root.selectedAssets[p]))
        }
    }

    StudioControls.MenuSeparator {
        visible: root.contextFilePath
        height: visible ? StudioTheme.Values.border : 0
    }

    StudioControls.MenuItem {
        text: qsTr("Rename Folder")
        visible: root.isDirContextMenu
        height: visible ? implicitHeight : 0
        onTriggered: renameFolderDialog.open()

        RenameFolderDialog {
            id: renameFolderDialog
        }
    }

    StudioControls.MenuItem {
        text: qsTr("New Folder")

        NewFolderDialog {
            id: newFolderDialog
        }

        onTriggered: newFolderDialog.open()
    }

    StudioControls.MenuItem {
        text: qsTr("Delete Folder")
        visible: root.isDirContextMenu
        height: visible ? implicitHeight : 0

        ConfirmDeleteFolderDialog {
            id: confirmDeleteFolderDialog
        }

        onTriggered: {
            var dirEmpty = !(root.contextDir.dirsModel && root.contextDir.dirsModel.rowCount() > 0)
                        && !(root.contextDir.filesModel && root.contextDir.filesModel.rowCount() > 0);

            if (dirEmpty)
                assetsModel.deleteFolder(root.contextDir.dirPath)
            else
                confirmDeleteFolderDialog.open()
        }
    }
}
