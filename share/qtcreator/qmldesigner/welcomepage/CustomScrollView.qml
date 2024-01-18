// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T

T.ScrollView {
    id: control

    property bool outsideHover: false

    hoverEnabled: true

    T.ScrollBar.vertical: CustomScrollBar {
        id: verticalScrollBar
        parent: control
        x: control.width + (verticalScrollBar.inUse ? 4 : 5)
        y: control.topPadding
        height: control.availableHeight
        orientation: Qt.Vertical

        show: (control.hovered || control.focus || control.outsideHover)
              && verticalScrollBar.isNeeded
    }
}
