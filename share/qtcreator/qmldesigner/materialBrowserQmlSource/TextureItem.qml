// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuickDesignerTheme
import HelperWidgets
import StudioTheme as StudioTheme

Rectangle {
    id: root

    visible: textureVisible

    color: "transparent"
    border.width: materialBrowserTexturesModel.selectedIndex === index
                        ? !rootView.materialSectionFocused ? 3 : 1 : 0
    border.color: materialBrowserTexturesModel.selectedIndex === index
                        ? StudioTheme.Values.themeControlOutlineInteraction
                        : "transparent"

    signal showContextMenu()

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: true

        onPressed: (mouse) => {
            rootView.focusMaterialSection(false)
            materialBrowserTexturesModel.selectTexture(index)

            if (mouse.button === Qt.LeftButton)
                rootView.startDragTexture(index, mapToGlobal(mouse.x, mouse.y))
            else if (mouse.button === Qt.RightButton)
                root.showContextMenu()
        }

        onDoubleClicked: materialBrowserTexturesModel.openTextureEditor();
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

    Image {
        source: "image://materialBrowserTex/" + textureSource
        asynchronous: true
        sourceSize.width: root.width - 10
        sourceSize.height: root.height - 10
        anchors.centerIn: parent
        smooth: true
    }
}
