// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import StudioTheme as StudioTheme

T.MenuItem {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property int labelSpacing: control.style.contextMenuLabelSpacing

    property alias buttonIcon: buttonIcon.text

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 0
    spacing: 0
    horizontalPadding: control.style.contextMenuHorizontalPadding

    indicator: Item {
        implicitWidth: control.style.controlSize.height
        implicitHeight: control.style.controlSize.height

        T.Label {
            id: buttonIcon
            anchors.fill: parent
            font {
                family: StudioTheme.Constants.iconFont.family
                pixelSize: control.style.baseIconFontSize
            }
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: control.enabled ? control.highlighted ? control.style.text.selectedText
                                                         : control.style.text.idle
                                    : control.style.text.disabled
        }
    }

    contentItem: Text {
        id: textLabel
        leftPadding: control.indicator.width
        text: control.text
        font: control.font
        color: control.enabled ? control.highlighted ? control.style.text.selectedText
                                                     : control.style.text.idle
                                : control.style.text.disabled
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        implicitWidth: textLabel.implicitWidth + control.labelSpacing
                       + control.leftPadding + control.rightPadding
        implicitHeight: control.style.controlSize.height
        x: control.style.borderWidth
        y: control.style.borderWidth
        width: (control.menu?.width ?? 0) - (control.style.borderWidth * 2)
        height: control.height - (control.style.borderWidth * 2)
        color: control.highlighted ? control.style.interaction : "transparent"
    }
}
