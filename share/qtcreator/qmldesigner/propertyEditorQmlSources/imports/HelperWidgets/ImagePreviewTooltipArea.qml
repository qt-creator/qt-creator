// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.15
import HelperWidgets 2.0

ToolTipArea {
    id: mouseArea

    property bool allowTooltip: true

    signal showContextMenu()

    function hide()
    {
        tooltipBackend.hideTooltip()
    }

    onExited: tooltipBackend.hideTooltip()
    onEntered: allowTooltip = true
    onCanceled: {
        tooltipBackend.hideTooltip()
        allowTooltip = true
    }
    onReleased: allowTooltip = true
    onPositionChanged: tooltipBackend.reposition()
    onClicked: function(mouse) {
        forceActiveFocus()
        if (mouse.button === Qt.RightButton)
            showContextMenu()
    }

    hoverEnabled: true
    acceptedButtons: Qt.LeftButton | Qt.RightButton

    Timer {
        interval: 1000
        running: mouseArea.containsMouse && mouseArea.allowTooltip
        onTriggered: {
            tooltipBackend.name = itemName
            tooltipBackend.path = componentPath
            tooltipBackend.showTooltip()
        }
    }
}
