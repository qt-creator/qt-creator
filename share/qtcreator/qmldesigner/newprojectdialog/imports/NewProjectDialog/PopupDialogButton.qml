// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import StudioTheme as StudioTheme

Button {
    id: root

    implicitWidth: Math.max(
                       background ? background.implicitWidth : 0,
                       textItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(
                        background ? background.implicitHeight : 0,
                        textItem.implicitHeight + topPadding + bottomPadding)
    leftPadding: 4
    rightPadding: 4

    background: Rectangle {
        id: background
        implicitWidth: 80
        implicitHeight: StudioTheme.Values.height
        color: StudioTheme.Values.themeControlBackground
        border.color: StudioTheme.Values.themeControlOutline
        anchors.fill: parent
    }

    contentItem: Text {
        id: textItem
        text: root.text
        font.family: StudioTheme.Constants.font.family
        font.pixelSize: DialogValues.defaultPixelSize
        color: StudioTheme.Values.themeTextColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        renderType: Text.QtRendering
        anchors.fill: parent
    }

    states: [
        State {
            name: "default"
            when: !root.hovered && !root.checked && !root.pressed

            PropertyChanges {
                target: background
                color: StudioTheme.Values.themeControlBackground
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: textItem
                color: StudioTheme.Values.themeTextColor
            }
        },
        State {
            name: "hover"
            when: root.hovered && !root.checked && !root.pressed

            PropertyChanges {
                target: background
                color: StudioTheme.Values.themeControlBackgroundHover
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: textItem
                color: StudioTheme.Values.themeTextColor
            }
        },
        State {
            name: "press"
            when: root.hovered && root.pressed

            PropertyChanges {
                target: background
                color: StudioTheme.Values.themeInteraction
                border.color: StudioTheme.Values.themeInteraction
            }
            PropertyChanges {
                target: textItem
                color: StudioTheme.Values.themeIconColor
            }
        }
    ]
}
