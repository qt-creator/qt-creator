// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

T.ScrollView {
    id: control

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
        parent: control
        x: control.width - verticalScrollBar.width - StudioTheme.Values.border
        y: StudioTheme.Values.border
        height: control.availableHeight - (2 * StudioTheme.Values.border)
                - (control.bothVisible ? control.horizontalThickness : 0)
        active: control.ScrollBar.horizontal.active
    }

    ScrollBar.horizontal: ScrollBar {
        id: horizontalScrollBar
        parent: control
        x: StudioTheme.Values.border
        y: control.height - horizontalScrollBar.height - StudioTheme.Values.border
        width: control.availableWidth - (2 * StudioTheme.Values.border)
               - (control.bothVisible ? control.verticalThickness : 0)
        active: control.ScrollBar.vertical.active
    }
}
