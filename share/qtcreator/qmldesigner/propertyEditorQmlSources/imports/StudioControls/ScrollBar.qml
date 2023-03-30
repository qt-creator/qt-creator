// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.ScrollBar {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    // This needs to be set, when using T.ScrollBar
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    active: true
    interactive: true
    visible: control.size < 1.0 || T.ScrollBar.AlwaysOn

    snapMode: T.ScrollBar.SnapAlways // TODO
    policy: T.ScrollBar.AsNeeded

    padding: 1 // TODO 0
    size: 1.0
    position: 1.0
    //orientation: Qt.Vertical

    contentItem: Rectangle {
        id: controlHandle
        implicitWidth: 4
        implicitHeight: 4
        radius: width / 2 // TODO 0
        color: control.style.scrollBar.handle
    }

    background: Rectangle {
        id: controlTrack
        color: control.style.scrollBar.track
    }
}
