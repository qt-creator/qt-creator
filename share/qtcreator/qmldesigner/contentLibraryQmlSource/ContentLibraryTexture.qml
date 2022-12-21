// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import QtQuick.Controls

import StudioTheme 1.0 as StudioTheme

Image {
    id: root

    source: modelData.textureIcon
    visible: modelData.textureVisible
    cache: false

    signal showContextMenu()

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: true

        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton)
                rootView.startDragTexture(modelData, mapToGlobal(mouse.x, mouse.y))
            else if (mouse.button === Qt.RightButton)
                root.showContextMenu()
        }
    }

    ToolTip {
        visible: mouseArea.containsMouse
        // contentWidth is not calculated correctly by the toolTip (resulting in a wider tooltip than
        // needed). Using a helper Text to calculate the correct width
        contentWidth: helperText.width
        bottomInset: -2
        text: modelData.textureToolTip
        delay: 1000

        Text {
            id: helperText
            text: modelData.textureToolTip
            visible: false
        }
    }
}
