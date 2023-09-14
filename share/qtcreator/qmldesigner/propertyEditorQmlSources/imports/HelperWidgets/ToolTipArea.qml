// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets

MouseArea {
    id: mouseArea

    Tooltip { id: myTooltip }

    onExited: myTooltip.hideText()
    onCanceled: myTooltip.hideText()
    onClicked: forceActiveFocus()

    hoverEnabled: true

    property string tooltip

    Timer {
        interval: 1000
        running: mouseArea.containsMouse && tooltip.length
        onTriggered: myTooltip.showText(mouseArea, Qt.point(mouseArea.mouseX, mouseArea.mouseY), tooltip)
    }
}
