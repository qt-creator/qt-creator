// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.MenuItem {
    id: control

    property alias shortcut: itemAction.shortcut

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property int labelSpacing: control.style.contextMenuLabelSpacing

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 0
    spacing: 0
    horizontalPadding: control.style.contextMenuHorizontalPadding
    action: Action {
        id: itemAction
    }

    contentItem: Item {
        Text {
            id: textLabel
            text: control.text
            font: control.font
            color: control.enabled ? control.highlighted ? control.style.text.selectedText
                                                         : control.style.text.idle
                                    : control.style.text.disabled
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            id: shortcutLabel
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: shortcutObserver.nativeText
            font: control.font
            color: textLabel.color

            Shortcut {
                id: shortcutObserver
                property int shortcutWorkaround: control.shortcut ?? 0
                sequence: shortcutObserver.shortcutWorkaround
            }
        }
    }

    arrow: T.Label {
        id: arrow
        x: parent.width - (control.style.controlSize.height + arrow.width) / 2
        y: (parent.height - arrow.height) / 2
        visible: control.subMenu
        text: StudioTheme.Constants.startNode
        color: control.style.icon.idle
        font.pixelSize: 8
        font.family: StudioTheme.Constants.iconFont.family
    }

    background: Rectangle {
        implicitWidth: textLabel.implicitWidth + control.labelSpacing + shortcutLabel.implicitWidth
                       + control.leftPadding + control.rightPadding
        implicitHeight: control.style.controlSize.height
        x: control.style.borderWidth
        y: control.style.borderWidth
        width: (control.menu?.width ?? 0) - (control.style.borderWidth * 2)
        height: control.height - (control.style.borderWidth * 2)
        color: control.highlighted ? control.style.interaction : "transparent"
    }
}
