// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import StudioControls as StudioControls
import StudioTheme as StudioTheme

StudioControls.Menu {
    id: root

    required property Item assetsView

    property bool _isDirectory: false
    property var _fileIndex: null
    property string _dirPath: ""
    property string _dirName: ""
    property var _onFolderCreated: null
    property var _onFolderRenamed: null
    property var _dirIndex: null
    property string _allExpandedState: ""
    property var _selectedAssetPathsList: null

    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

    function openContextMenuForRoot(rootModelIndex, dirPath, dirName, onFolderCreated)
    {
        root._onFolderCreated = onFolderCreated
        root._fileIndex = ""
        root._dirPath = dirPath
        root._dirName = dirName
        root._dirIndex = rootModelIndex
        root._isDirectory = false
        root.popup()
    }

    function openContextMenuForDir(dirModelIndex, dirPath, dirName, allExpandedState,
                                   onFolderCreated, onFolderRenamed)
    {
        root._onFolderCreated = onFolderCreated
        root._onFolderRenamed = onFolderRenamed
        root._dirPath = dirPath
        root._dirName = dirName
        root._fileIndex = ""
        root._dirIndex = dirModelIndex
        root._isDirectory = true
        root._allExpandedState = allExpandedState
        root.popup()
    }

    function openContextMenuForFile(fileIndex, dirModelIndex, selectedAssetPathsList)
    {
        var numSelected = selectedAssetPathsList.filter(p => p).length
        deleteFileItem.text = numSelected > 1 ? qsTr("Delete Files") : qsTr("Delete File")

        root._selectedAssetPathsList = selectedAssetPathsList
        root._fileIndex = fileIndex
        root._dirIndex = dirModelIndex
        root._dirPath = assetsModel.filePath(dirModelIndex)
        root._isDirectory = false
        root.popup()
    }

    StudioControls.MenuItem {
        text: qsTr("Expand All")
        enabled: root._allExpandedState !== "all_expanded"
        visible: root._isDirectory
        height: visible ? implicitHeight : 0
        onTriggered: root.assetsView.expandAll()
    }

    StudioControls.MenuItem {
        text: qsTr("Collapse All")
        enabled: root._allExpandedState !== "all_collapsed"
        visible: root._isDirectory
        height: visible ? implicitHeight : 0
        onTriggered: root.assetsView.collapseAll()
    }

    StudioControls.MenuSeparator {
        visible: root._isDirectory
        height: visible ? StudioTheme.Values.border : 0
    }

    StudioControls.MenuItem {
        id: deleteFileItem
        text: qsTr("Delete File")
        visible: root._fileIndex
        height: deleteFileItem.visible ? deleteFileItem.implicitHeight : 0
        onTriggered: {
            let deleted = assetsModel.requestDeleteFiles(root._selectedAssetPathsList)
            if (!deleted)
                confirmDeleteFiles.open()
        }

        ConfirmDeleteFilesDialog {
            id: confirmDeleteFiles
            parent: root.assetsView
            files: root._selectedAssetPathsList

            onAccepted: root.assetsView.selectedAssets = {}
        }
    }

    StudioControls.MenuSeparator {
        visible: root._fileIndex
        height: visible ? StudioTheme.Values.border : 0
    }

    StudioControls.MenuItem {
        text: qsTr("Rename Folder")
        visible: root._isDirectory
        height: visible ? implicitHeight : 0
        onTriggered: renameFolderDialog.open()

        RenameFolderDialog {
            id: renameFolderDialog
            parent: root.assetsView
            dirPath: root._dirPath
            dirName: root._dirName

            onAccepted: root._onFolderRenamed()
        }
    }

    StudioControls.MenuItem {
        text: qsTr("New Folder")

        NewFolderDialog {
            id: newFolderDialog
            parent: root.assetsView
            dirPath: root._dirPath

            onAccepted: root._onFolderCreated(newFolderDialog.createdDirPath)
        }

        onTriggered: newFolderDialog.open()
    }

    StudioControls.MenuItem {
        text: qsTr("Delete Folder")
        visible: root._isDirectory
        height: visible ? implicitHeight : 0

        ConfirmDeleteFolderDialog {
            id: confirmDeleteFolderDialog
            parent: root.assetsView
            dirName: root._dirName
            dirIndex: root._dirIndex
        }

        onTriggered: {
            if (!assetsModel.hasChildren(root._dirIndex)) {
                // NOTE: the folder may still not be empty -- it doesn't have files visible to the
                // user, but that doesn't mean that there are no other files (e.g. files of unknown
                // types) on disk in this directory.
                assetsModel.deleteFolderRecursively(root._dirIndex)
            } else {
                confirmDeleteFolderDialog.open()
            }
        }
    }
}
