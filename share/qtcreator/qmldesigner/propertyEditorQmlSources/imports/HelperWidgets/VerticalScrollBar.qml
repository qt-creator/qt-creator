// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls.Basic
import StudioTheme as StudioTheme

ScrollBar {
    id: scrollBar

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    property bool scrollBarVisible: parent.childrenRect.height > parent.height

    minimumSize: orientation == Qt.Horizontal ? height / width : width / height

    orientation: Qt.Vertical
    policy: scrollBar.scrollBarVisible ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
    x: parent.width - width
    y: 0
    height: parent.availableHeight
            - (parent.bothVisible ? parent.horizontalThickness : 0)
    padding: 0

    background: Rectangle {
        color: StudioTheme.Values.themeScrollBarTrack
    }

    contentItem: Rectangle {
        implicitWidth: StudioTheme.Values.scrollBarThickness
        color: StudioTheme.Values.themeScrollBarHandle
    }
}
