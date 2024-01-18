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

    visible: textureVisible

    signal showContextMenu()

    function forceFinishEditing() {
        txtId.commitRename()
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: true

        onPressed: (mouse) => {
            MaterialBrowserBackend.materialBrowserTexturesModel.selectTexture(index)
            MaterialBrowserBackend.rootView.focusMaterialSection(false)

            if (mouse.button === Qt.LeftButton)
                MaterialBrowserBackend.rootView.startDragTexture(index, mapToGlobal(mouse.x, mouse.y))
            else if (mouse.button === Qt.RightButton)
                root.showContextMenu()
        }

        onDoubleClicked: MaterialBrowserBackend.materialBrowserTexturesModel.openTextureEditor();
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
            id: txtId

            text: textureId
            width: img.width
            anchors.horizontalCenter: parent.horizontalCenter

            onRenamed: (newId) => {
                MaterialBrowserBackend.materialBrowserTexturesModel.setTextureId(index, newId);
                mouseArea.forceActiveFocus()
            }

            onClicked: {
                MaterialBrowserBackend.materialBrowserTexturesModel.selectTexture(index)
                MaterialBrowserBackend.rootView.focusMaterialSection(false)
            }
        }
    }

    Rectangle {
        id: marker
        anchors.fill: parent

        color: "transparent"
        border.width: MaterialBrowserBackend.materialBrowserTexturesModel.selectedIndex === index
                            ? !MaterialBrowserBackend.rootView.materialSectionFocused ? 3 : 1 : 0
        border.color: MaterialBrowserBackend.materialBrowserTexturesModel.selectedIndex === index
                            ? StudioTheme.Values.themeControlOutlineInteraction
                            : "transparent"
    }
}
