// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme as StudioTheme
import CollectionEditorBackend

ListView {
    id: root

    model: CollectionEditorBackend.model
    clip: true

    function closeDialogs() {
        currentCollection.dereference()
        collectionMenu.close()
        deleteDialog.reject()
        renameDialog.reject()
    }

    function deleteCurrentCollection() {
        deleteDialog.open()
    }

    delegate: CollectionItem {
        implicitWidth: root.width
        onDeleteItem: root.model.removeRow(index)
        onContextMenuRequested: collectionMenu.openMenu(this)
    }

    QtObject {
        id: currentCollection

        property CollectionItem item
        readonly property string name: item ? item.name : ""
        readonly property bool selected: item ? item.isSelected : false
        readonly property int index: item ? item.id : -1

        function updateItem() {
            currentCollection.item = collectionMenu.clickedItem ?? root.itemAtIndex(root.model.selectedIndex)
        }

        function rename(newName) {
            if (item)
                item.rename(newName)
        }

        function deleteItem() {
            if (item)
                item.deleteItem()
        }

        function dereference() {
            item = null
        }
    }

    StudioControls.Menu {
        id: collectionMenu

        property CollectionItem clickedItem

        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        enabled: root.count

        function openMenu(item) {
            collectionMenu.clickedItem = item
            currentCollection.updateItem()
            collectionMenu.popup()
        }

        onClosed: collectionMenu.clickedItem = null

        Action {
            id: menuDeleteAction

            text: qsTr("Delete")
            onTriggered: deleteDialog.open()
        }

        Action {
            id: menuRenameAction

            text: qsTr("Rename")
            onTriggered: renameDialog.open()
        }

        Action {
            id: menuAssignAction

            text: qsTr("Assign to the selected node")
            enabled: CollectionEditorBackend.rootView.targetNodeSelected
            onTriggered: rootView.assignCollectionToSelectedNode(currentCollection.name)
        }
    }

    ConfirmDeleteCollectionDialog {
        id: deleteDialog

        collectionName: currentCollection.name
        onAboutToShow: currentCollection.updateItem()
        onAccepted: currentCollection.deleteItem()
    }

    RenameCollectionDialog {
        id: renameDialog

        collectionName: currentCollection.name
        onAboutToShow: currentCollection.updateItem()
        onAccepted: currentCollection.rename(renameDialog.newCollectionName)
    }

    Connections {
        target: root.model

        function onModelReset() {
            root.closeDialogs()
        }
    }
}
