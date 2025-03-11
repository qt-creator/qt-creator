// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme
import MaterialBrowserBackend

Item {
    id: root

    readonly property bool selected: textureSelected?? false
    readonly property bool matchedSearch: textureMatchedSearch?? false
    readonly property int itemIndex: index
    property bool rightClicked: false

    visible: matchedSearch

    signal showContextMenu()

    function forceFinishEditing() {
        txtName.commitRename()
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: true

        function handleClick(mouse) {
            MaterialBrowserBackend.rootView.focusMaterialSection(false)

            if (mouse.button === Qt.LeftButton) {
                let appendTexture = mouse.modifiers & Qt.ControlModifier
                MaterialBrowserBackend.materialBrowserTexturesModel.selectTexture(index, appendTexture)
                MaterialBrowserBackend.rootView.startDragTexture(index, mapToGlobal(mouse.x, mouse.y))
            } else if (mouse.button === Qt.RightButton) {
                root.showContextMenu()
            }
        }

        onPressed: (mouse) => handleClick(mouse)

        onDoubleClicked: {
            MaterialBrowserBackend.materialBrowserTexturesModel.selectTexture(index, false)
            MaterialBrowserBackend.rootView.openPropertyEditor()
        }
    }

    ToolTip {
        visible: mouseArea.containsMouse
        // contentWidth is not calculated correctly by the toolTip (resulting in a wider tooltip than
        // needed). Using a helper Text to calculate the correct width
        contentWidth: helperText.width
        bottomInset: -2
        text: textureToolTip
        delay: 1000

        Text {
            id: helperText
            text: textureToolTip
            visible: false
        }
    }

    Column {
        anchors.fill: parent
        spacing: 1

        Image {
            id: img
            source: "image://materialBrowserTex/" + textureSource
            asynchronous: true
            width: root.width
            height: img.width
            anchors.horizontalCenter: parent.horizontalCenter
            smooth: true
            fillMode: Image.PreserveAspectFit
        }

        // Eat keys so they are not passed to parent while editing name
        Keys.onPressed: (event) => {
            event.accepted = true
        }

        MaterialBrowserItemName {
            id: txtName

            text: textureName
            width: img.width
            anchors.horizontalCenter: parent.horizontalCenter

            onRenamed: (newName) => {
                MaterialBrowserBackend.materialBrowserTexturesModel.setTextureName(index, newName)
                mouseArea.forceActiveFocus()
            }

            onClicked: (mouse) => mouseArea.handleClick(mouse)
        }
    }

    ItemBorder {
        selected: root.selected
        rightClicked: root.rightClicked
    }
}
