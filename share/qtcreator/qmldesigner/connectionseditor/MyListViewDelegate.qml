// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls

ItemDelegate {
    id: control
    hoverEnabled: true

    contentItem: Text {
        leftPadding: 8
        rightPadding: 8
        text: control.text
        font: control.font
        opacity: enabled ? 1.0 : 0.3
        color: control.hovered ? "#111111" : "white"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 30
        opacity: enabled ? 1 : 0.3
        color: control.hovered ? "#4DBFFF" : "transparent"
    }
}
