// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls

import QtCreator.Tracing

ToolButton {
    implicitWidth: 30

    property string imageSource

    ToolTip.visible: enabled && hovered
    ToolTip.delay: 1000

    Image {
        source: parent.enabled ? parent.imageSource : parent.imageSource + "/disabled"
        width: 16
        height: 16
        anchors.centerIn: parent
        smooth: false
    }

    background: PaddedRectangle {
        padding: Theme.compactToolbar() ? 0 : 3
        radius: Theme.compactToolbar() ? 0 : 5
        color: (parent.checked || parent.pressed)
               ? Theme.color(Theme.FancyToolButtonSelectedColor)
               : (parent.hovered && parent.enabled)
                 ? Theme.color(Theme.FancyToolButtonHoverColor)
                 : "#00000000"
    }
}
