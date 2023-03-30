// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.ToolTip {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    x: parent ? (parent.width - control.implicitWidth) / 2 : 0
    y: -control.implicitHeight - 3

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    margins: 6
    padding: 4
    delay: 1000
    timeout: 5000
    closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutsideParent
                 | T.Popup.CloseOnReleaseOutsideParent

    contentItem: Text {
        text: control.text
        wrapMode: Text.Wrap
        font.pixelSize: control.style.baseFontSize
        color: control.style.toolTip.text
    }

    background: Rectangle {
        color: control.style.toolTip.background
        border.width: control.style.borderWidth
        border.color: control.style.toolTip.border
    }
}

