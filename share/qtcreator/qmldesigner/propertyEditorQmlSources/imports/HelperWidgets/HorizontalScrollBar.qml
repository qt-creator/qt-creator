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

    property bool scrollBarVisible: parent.childrenRect.width > parent.width

    minimumSize: orientation == Qt.Horizontal ? height / width : width / height

    orientation: Qt.Horizontal
    policy: scrollBar.scrollBarVisible ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
    x: 0
    y: parent.height - height
    width: parent.availableWidth
           - (parent.bothVisible ? parent.verticalThickness : 0)
    padding: 0

    background: Rectangle {
        color: StudioTheme.Values.themeScrollBarTrack
    }

    contentItem: Rectangle {
        implicitHeight: StudioTheme.Values.scrollBarThickness
        color: StudioTheme.Values.themeScrollBarHandle
    }
}
