// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts

MouseArea {
    id: root

    ToolTipExt { id: toolTip }

    onExited: toolTip.hideText()
    onCanceled: toolTip.hideText()
    onClicked: root.forceActiveFocus()

    hoverEnabled: true

    property string text

    Timer {
        interval: 1000
        running: root.containsMouse && root.text.length
        onTriggered: toolTip.showText(root, Qt.point(root.mouseX, root.mouseY), root.text)
    }
}
