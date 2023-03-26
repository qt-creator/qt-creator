// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Item {
    property alias currentColor: colorLine.color

    width: 300
    height: StudioTheme.Values.colorEditorPopupLineHeight

    Image {
        id: checkerboard
        anchors.fill: colorLine
        source: "images/checkers.png"
        fillMode: Image.Tile
    }

    Rectangle {
        id: colorLine
        height: StudioTheme.Values.hueSliderHeight
        width: parent.width
        border.color: StudioTheme.Values.themeControlOutline
        border.width: StudioTheme.Values.border
        color: "white"
        anchors.bottom: parent.bottom
    }
}
