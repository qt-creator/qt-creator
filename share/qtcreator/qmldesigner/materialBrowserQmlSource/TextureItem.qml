// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuickDesignerTheme
import HelperWidgets
import StudioTheme as StudioTheme

Image {
    id: root

    source: textureSource
    sourceSize.width: root.width
    sourceSize.height: root.height
    visible: textureVisible
    cache: false

    signal showContextMenu()

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton)
                rootView.startDragTexture(modelData, mapToGlobal(mouse.x, mouse.y))
            else if (mouse.button === Qt.RightButton)
                root.showContextMenu()
        }
    }
}
