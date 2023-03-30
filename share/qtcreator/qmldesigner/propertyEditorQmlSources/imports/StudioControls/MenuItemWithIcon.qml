// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.MenuItem {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property int labelSpacing: control.style.contextMenuLabelSpacing

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(
                        implicitBackgroundHeight + topInset + bottomInset,
                        implicitContentHeight + topPadding + bottomPadding,
                        implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 0
    spacing: 0
    horizontalPadding: control.style.contextMenuHorizontalPadding
    action: Action {}

    contentItem: Item {
        Text {
            id: iconLabel
            text: control.checked ? StudioTheme.Constants.tickIcon : ""
            visible: true
            color: control.enabled ? control.highlighted ? control.style.text.selectedText : control.style.text.idle : control.style.text.disabled
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: control.style.baseIconFontSize
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            id: textLabel
            x: control.style.squareControlSize.width
            text: control.text
            font: control.font
            color: control.enabled ? control.highlighted ? control.style.text.selectedText : control.style.text.idle : control.style.text.disabled
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    background: Rectangle {
        implicitWidth: iconLabel.implicitWidth + textLabel.implicitWidth + control.labelSpacing
                       + control.leftPadding + control.rightPadding
        implicitHeight: control.style.controlSize.height
        x: control.style.borderWidth
        y: control.style.borderWidth
        width: control.menu.width - (control.style.borderWidth * 2)
        height: control.height - (control.style.borderWidth * 2)
        color: control.highlighted ? control.style.interaction : "transparent"
    }
}
