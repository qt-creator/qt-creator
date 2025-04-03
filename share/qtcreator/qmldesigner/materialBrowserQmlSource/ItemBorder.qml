// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Shapes
import StudioTheme as StudioTheme

Shape {
    id: root

    property bool selected: false
    property bool rightClicked: false

    anchors.fill: parent

    ShapePath {
        strokeWidth: root.rightClicked ? 2 : 0
        fillColor: "transparent"
        strokeColor: StudioTheme.Values.themeInteractionHover
        strokeStyle: ShapePath.DashLine
        dashPattern: [ 2, 4 ]
        PathRectangle {
            width: root.width
            height: root.height
        }
    }

    ShapePath {
        strokeWidth: root.selected ? 1 : 0
        strokeColor: StudioTheme.Values.themeControlOutlineInteraction
        fillColor: "transparent"
        strokeStyle: ShapePath.PathLinear
        PathRectangle {
            width: root.width
            height: root.height
        }
    }
}
