// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

Row {
    id: myButtonRow

    property bool hover: (actionIndicator.hover || myButtonRow.childHover) && myButtonRow.enabled
    property bool childHover: false

    property alias actionIndicator: actionIndicator

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: StudioTheme.Values.actionIndicatorWidth
    property real __actionIndicatorHeight: StudioTheme.Values.actionIndicatorHeight

    ActionIndicator {
        id: actionIndicator
        myControl: myButtonRow
        x: 0
        y: 0
        // + StudioTheme.Values.border on width because of negative spacing on the row
        width: actionIndicator.visible ? myButtonRow.__actionIndicatorWidth + StudioTheme.Values.border : 0
        height: actionIndicator.visible ? myButtonRow.__actionIndicatorHeight : 0

        onHoverChanged: myButtonRow.hoverCallback()
    }

    spacing: -StudioTheme.Values.border

    function hoverCallback() {
        var hover = false

        for (var i = 0; i < children.length; ++i) {
            if (children[i].hover !== undefined)
                hover = hover || children[i].hover
        }

        myButtonRow.childHover = hover
    }

    onHoverChanged: {
        for (var i = 0; i < children.length; ++i)
            if (children[i].globalHover !== undefined)
                children[i].globalHover = myButtonRow.hover
    }
}
