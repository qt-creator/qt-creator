// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T

import StudioTheme as StudioTheme

T.ItemDelegate {
    id: root

    property StudioTheme.ControlStyle style: StudioTheme.Values.toolbarButtonStyle

    property alias myIcon: iconLabel.text
    property alias myText: textLabel.text

    //width: root.menuWidth - 2 * window.padding
    //height: root.style.controlSize.height// - 2 * root.style.borderWidth

    contentItem: Row {
        id: row
        width: root.width
        height: root.height
        spacing: 0

        T.Label {
            id: iconLabel
            width: root.height
            height: root.height
            color: {
                if (root.checked)
                    return root.style.icon.selected

                if (!root.enabled)
                    return root.style.icon.disabled

                return root.style.icon.idle
            }
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: root.style.baseIconFontSize
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            text: StudioTheme.Constants.playOutline_medium
        }

        T.Label {
            id: textLabel
            width: root.width - row.spacing - iconLabel.width
            height: root.height
            color: {
                if (!root.enabled && root.checked)
                    return root.style.text.idle

                if (root.checked)
                    return root.style.text.selectedText

                if (!root.enabled)
                    return root.style.text.disabled

                return root.style.text.idle
            }
            elide: Text.ElideRight
            font.pixelSize: root.style.baseFontSize
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
        }
    }

    background: Rectangle {
        id: rootBackground
        x: 0
        y: 0
        width: root.width
        height: root.height
        opacity: enabled ? 1 : 0.3

        color: {
            if (!root.enabled && root.checked)
                return root.style.interaction

            if (!root.enabled)
                return "transparent"

            if (root.hovered && root.checked)
                return root.style.interactionHover

            if (root.checked)
                return root.style.interaction

            if (root.hovered)
                return root.style.background.hover

            return "transparent"
        }
    }
}
