// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.TabButton {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 4

    background: Rectangle {
        id: buttonBackground
        color: control.checked ? control.style.interaction : "transparent"
        border.width: control.style.borderWidth
        border.color: control.style.interaction
    }

    contentItem: T.Label {
        id: buttonIcon
        color: control.checked ? control.style.background.idle : control.style.interaction
        font.pixelSize: control.style.baseFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        anchors.fill: parent
        renderType: Text.QtRendering
        text: control.text
    }
}
