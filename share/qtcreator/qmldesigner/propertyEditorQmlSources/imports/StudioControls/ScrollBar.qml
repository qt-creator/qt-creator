// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

T.ScrollBar {
    id: control

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
        color: StudioTheme.Values.themeScrollBarHandle
    }

    background: Rectangle {
        id: controlTrack
        color: StudioTheme.Values.themeScrollBarTrack
    }
}
