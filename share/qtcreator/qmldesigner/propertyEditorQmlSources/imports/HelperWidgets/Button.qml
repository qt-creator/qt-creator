// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

T.AbstractButton {
    id: myButton

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    background: Rectangle {
        id: buttonBackground
        implicitWidth: 100
        implicitHeight: 23
        radius: 3
        color: StudioTheme.Values.themeControlBackground
        border.color: StudioTheme.Values.themeControlOutline
        border.width: StudioTheme.Values.border
    }

    contentItem: Text {
        id: buttonText
        color: StudioTheme.Values.themeTextColor
        font.family: StudioTheme.Constants.font.family
        font.pixelSize: StudioTheme.Values.myFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        anchors.fill: parent
        renderType: Text.QtRendering
        text: myButton.text
    }

    states: [
        State {
            name: "default"
            when: myButton.enabled && !myButton.hovered && !myButton.pressed
                  && !myButton.checked
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackground
            }
        },
        State {
            name: "hovered"
            when: myButton.enabled && myButton.hovered && !myButton.pressed
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackgroundHover
            }
        },
        State {
            name: "pressed"
            when: myButton.enabled && myButton.hovered && myButton.pressed
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackgroundInteraction
                border.color: StudioTheme.Values.themeControlOutlineInteraction
            }
        },
        State {
            name: "disabled"
            when: !myButton.enabled
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackgroundDisabled
                border.color: StudioTheme.Values.themeControlOutlineDisabled
            }
            PropertyChanges {
                target: buttonText
                color: StudioTheme.Values.themeTextColorDisabled
            }
        }
    ]
}
