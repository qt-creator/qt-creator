// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Controls 2.15
import StudioTheme 1.0 as StudioTheme

ScrollBar {
    id: scrollBar

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    property bool scrollBarVisible: parent.contentHeight > scrollBar.height

    minimumSize: scrollBar.width / scrollBar.height
    orientation: Qt.Vertical
    policy: scrollBar.scrollBarVisible ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff

    height: parent.availableHeight
            - (parent.bothVisible ? parent.horizontalThickness : 0)
    padding: scrollBar.active ? StudioTheme.Values.scrollBarActivePadding
                              : StudioTheme.Values.scrollBarInactivePadding

    background: Rectangle {
        implicitWidth: StudioTheme.Values.scrollBarThickness
        implicitHeight: StudioTheme.Values.scrollBarThickness
        color: StudioTheme.Values.themeScrollBarTrack
    }

    contentItem: Rectangle {
        implicitWidth: StudioTheme.Values.scrollBarThickness - 2 * scrollBar.padding
        implicitHeight: StudioTheme.Values.scrollBarThickness - 2 * scrollBar.padding
        color: StudioTheme.Values.themeScrollBarHandle
    }
}
