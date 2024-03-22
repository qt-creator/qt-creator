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

        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        function openMenu(item) {
            currentCollection.item = item
            popup()
        }

        StudioControls.MenuItem {
            text: qsTr("Delete")
            shortcut: StandardKey.Delete
            onTriggered: deleteDialog.open()
        }

        StudioControls.MenuItem {
            text: qsTr("Rename")
            shortcut: StandardKey.Replace
            onTriggered: renameDialog.open()
        }

        StudioControls.MenuItem {
            text: qsTr("Assign to the selected node")
            enabled: CollectionEditorBackend.rootView.targetNodeSelected
            onTriggered: rootView.assignCollectionToSelectedNode(currentCollection.name)
        }
    }

    StudioControls.Dialog {
        id: deleteDialog

        title: qsTr("Deleting the model")
        clip: true

        onAccepted: currentCollection.deleteItem()

        contentItem: ColumnLayout {
            id: deleteDialogContent // Keep the id here even if it's not used, because the dialog might lose implicitSize

            width: 300
            spacing: 2

            Text {
                Layout.fillWidth: true

                wrapMode: Text.WordWrap
                color: StudioTheme.Values.themeTextColor
                text: qsTr("Are you sure that you want to delete model \"%1\"?"
                           + "\nThe model will be deleted permanently.").arg(currentCollection.name)

            }

            Spacer {}

            RowLayout {
                spacing: StudioTheme.Values.sectionRowSpacing
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                HelperWidgets.Button {
                    text: qsTr("Delete")
                    onClicked: deleteDialog.accept()
                }

                HelperWidgets.Button {
                    text: qsTr("Cancel")
                    onClicked: deleteDialog.reject()
                }
            }
        }
    }

    StudioControls.Dialog {
        id: renameDialog

        title: qsTr("Rename model")

        onAccepted: {
            if (newNameField.text !== "")
                currentCollection.rename(newNameField.text)
        }

        onOpened: {
            newNameField.text = currentCollection.name
        }

        contentItem: ColumnLayout {
            spacing: 2

            Text {
                text: qsTr("Previous name: " + currentCollection.name)
                color: StudioTheme.Values.themeTextColor
            }

            Spacer {}

            Text {
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                text: qsTr("New name:")
                color: StudioTheme.Values.themeTextColor
            }

            StudioControls.TextField {
                id: newNameField

                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                Layout.fillWidth: true

                actionIndicator.visible: false
                translationIndicator.visible: false
                validator: newNameValidator

                Keys.onEnterPressed: renameDialog.accept()
                Keys.onReturnPressed: renameDialog.accept()
                Keys.onEscapePressed: renameDialog.reject()

                onTextChanged: {
                    btnRename.enabled = newNameField.text !== ""
                }
            }

            Spacer {}

            RowLayout {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                spacing: StudioTheme.Values.sectionRowSpacing

                HelperWidgets.Button {
                    id: btnRename

                    text: qsTr("Rename")
                    onClicked: renameDialog.accept()
                }

                HelperWidgets.Button {
                    text: qsTr("Cancel")
                    onClicked: renameDialog.reject()
                }
            }
        }
    }

    Connections {
        target: root.model

        function onModelReset() {
            root.closeDialogs()
        }
    }

    RegularExpressionValidator {
        id: newNameValidator
        regularExpression: /^\w+$/
    }

    component Spacer: Item {
        implicitWidth: 1
        implicitHeight: StudioTheme.Values.columnGap
    }
}
