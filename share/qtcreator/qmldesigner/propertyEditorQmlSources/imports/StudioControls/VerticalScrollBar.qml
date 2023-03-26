// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
//import QtQuick.Controls
import StudioTheme 1.0 as StudioTheme

ScrollBar {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    property bool scrollBarVisible: parent.contentHeight > control.height

    minimumSize: control.width / control.height
    orientation: Qt.Vertical
    policy: control.scrollBarVisible ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff

    height: parent.availableHeight
            - (parent.bothVisible ? parent.horizontalThickness : 0)
    padding: control.active ? control.style.scrollBarActivePadding
                            : control.style.scrollBarInactivePadding

    background: Rectangle {
        implicitWidth: control.style.scrollBarThickness
        implicitHeight: control.style.scrollBarThickness
        color: control.style.scrollBar.track
    }

    contentItem: Rectangle {
        implicitWidth: control.style.scrollBarThickness - 2 * control.padding
        implicitHeight: control.style.scrollBarThickness - 2 * control.padding
        color: control.style.scrollBar.handle
    }
}
