// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.ScrollView {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property alias horizontalThickness: horizontalScrollBar.height
    property alias verticalThickness: verticalScrollBar.width
    property bool bothVisible: verticalScrollBar.visible
                               && horizontalScrollBar.visible

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    ScrollBar.vertical: ScrollBar {
        id: verticalScrollBar
        style: control.style
        parent: control
        x: control.width - verticalScrollBar.width - control.style.borderWidth
        y: control.style.borderWidth
        height: control.availableHeight - (2 * control.style.borderWidth)
                - (control.bothVisible ? control.horizontalThickness : 0)
        active: control.ScrollBar.horizontal.active
    }

    ScrollBar.horizontal: ScrollBar {
        id: horizontalScrollBar
        style: control.style
        parent: control
        x: control.style.borderWidth
        y: control.height - horizontalScrollBar.height - control.style.borderWidth
        width: control.availableWidth - (2 * control.style.borderWidth)
               - (control.bothVisible ? control.verticalThickness : 0)
        active: control.ScrollBar.vertical.active
    }
}
