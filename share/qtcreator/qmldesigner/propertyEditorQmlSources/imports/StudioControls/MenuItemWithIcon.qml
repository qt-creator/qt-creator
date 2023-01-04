// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

T.MenuItem {
    id: control

    property int labelSpacing: StudioTheme.Values.contextMenuLabelSpacing

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(
                        implicitBackgroundHeight + topInset + bottomInset,
                        implicitContentHeight + topPadding + bottomPadding,
                        implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 0
    spacing: 0
    horizontalPadding: StudioTheme.Values.contextMenuHorizontalPadding
    action: Action {}

    contentItem: Item {
        Text {
            id: iconLabel
            text: control.checked ? StudioTheme.Constants.tickIcon : ""
            visible: true
            color: control.enabled ? StudioTheme.Values.themeTextColor : StudioTheme.Values.themeTextColorDisabled
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: StudioTheme.Values.myIconFontSize
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            id: textLabel
            x: StudioTheme.Values.height
            text: control.text
            font: control.font
            color: control.enabled ? StudioTheme.Values.themeTextColor : StudioTheme.Values.themeTextColorDisabled
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    background: Rectangle {
        implicitWidth: iconLabel.implicitWidth + textLabel.implicitWidth + control.labelSpacing
                       + control.leftPadding + control.rightPadding
        implicitHeight: StudioTheme.Values.height
        x: StudioTheme.Values.border
        y: StudioTheme.Values.border
        width: control.menu.width - (StudioTheme.Values.border * 2)
        height: control.height - (StudioTheme.Values.border * 2)
        color: control.down ? control.palette.midlight : control.highlighted ? StudioTheme.Values.themeInteraction : "transparent"
    }
}
