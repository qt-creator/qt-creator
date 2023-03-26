// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

Row {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property bool hover: (actionIndicator.hover || control.childHover) && control.enabled
    property bool childHover: false

    property alias actionIndicator: actionIndicator

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: control.style.actionIndicatorSize.width
    property real __actionIndicatorHeight: control.style.actionIndicatorSize.height

    ActionIndicator {
        id: actionIndicator
        style: control.style
        __parentControl: control
        x: 0
        y: 0
        // + borderWidth because of negative spacing on the row
        width: actionIndicator.visible ? control.__actionIndicatorWidth + control.style.borderWidth : 0
        height: actionIndicator.visible ? control.__actionIndicatorHeight : 0

        onHoverChanged: control.hoverCallback()
    }

    spacing: -control.style.borderWidth

    function hoverCallback() {
        var hover = false

        for (var i = 0; i < control.children.length; ++i) {
            if (control.children[i].hover !== undefined)
                hover = hover || control.children[i].hover
        }

        control.childHover = hover
    }

    onHoverChanged: {
        for (var i = 0; i < control.children.length; ++i)
            if (control.children[i].globalHover !== undefined)
                control.children[i].globalHover = control.hover
    }
}
