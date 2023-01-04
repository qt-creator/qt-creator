// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

T.AbstractButton {
    id: myButton

    property bool globalHover: false
    property bool hover: myButton.hovered

    property alias buttonIcon: buttonIcon.text
    property alias iconColor: buttonIcon.color
    property alias iconFont: buttonIcon.font.family
    property alias iconSize: buttonIcon.font.pixelSize
    property alias iconItalic: buttonIcon.font.italic
    property alias iconBold: buttonIcon.font.bold
    property alias iconRotation: buttonIcon.rotation
    property alias backgroundVisible: buttonBackground.visible
    property alias backgroundRadius: buttonBackground.radius

    // Inverts the checked style
    property bool checkedInverted: false

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    height: StudioTheme.Values.height
    width: StudioTheme.Values.height
    z: myButton.checked ? 10 : 3
    activeFocusOnTab: false

    onHoverChanged: {
        if (parent !== undefined && parent.hoverCallback !== undefined && myButton.enabled)
            parent.hoverCallback()
    }

    background: Rectangle {
        id: buttonBackground
        color: StudioTheme.Values.themeControlBackground
        border.color: StudioTheme.Values.themeControlOutline
        border.width: StudioTheme.Values.border
    }

    indicator: Item {
        x: 0
        y: 0
        width: myButton.width
        height: myButton.height

        T.Label {
            id: buttonIcon
            color: StudioTheme.Values.themeTextColor
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: StudioTheme.Values.myIconFontSize
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            anchors.fill: parent
            renderType: Text.QtRendering

            states: [
                State {
                    name: "default"
                    when: myButton.enabled && !myButton.pressed && !myButton.checked
                    PropertyChanges {
                        target: buttonIcon
                        color: StudioTheme.Values.themeIconColor
                    }
                },
                State {
                    name: "press"
                    when: myButton.enabled && myButton.pressed
                    PropertyChanges {
                        target: buttonIcon
                        color: StudioTheme.Values.themeIconColor
                    }
                },
                State {
                    name: "check"
                    when: myButton.enabled && !myButton.pressed && myButton.checked
                    PropertyChanges {
                        target: buttonIcon
                        color: myButton.checkedInverted ? StudioTheme.Values.themeTextSelectedTextColor
                                                        : StudioTheme.Values.themeIconColorSelected
                    }
                },
                State {
                    name: "disable"
                    when: !myButton.enabled
                    PropertyChanges {
                        target: buttonIcon
                        color: StudioTheme.Values.themeTextColorDisabled
                    }
                }
            ]
        }
    }

    states: [
        State {
            name: "default"
            when: myButton.enabled && !myButton.globalHover && !myButton.hover
                  && !myButton.pressed && !myButton.checked
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackground
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: myButton
                z: 3
            }
        },
        State {
            name: "globalHover"
            when: myButton.globalHover && !myButton.hover && !myButton.pressed && myButton.enabled
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackground
                border.color: StudioTheme.Values.themeControlOutline
            }
        },
        State {
            name: "hover"
            when: !myButton.checked && myButton.hover && !myButton.pressed && myButton.enabled
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackgroundHover
                border.color: StudioTheme.Values.themeControlOutline
            }
        },
        State {
            name: "hoverCheck"
            when: myButton.checked && myButton.hover && !myButton.pressed && myButton.enabled
            PropertyChanges {
                target: buttonBackground
                color: myButton.checkedInverted ? StudioTheme.Values.themeInteractionHover
                                                : StudioTheme.Values.themeControlBackgroundHover
                border.color: myButton.checkedInverted ? StudioTheme.Values.themeInteractionHover
                                                       : StudioTheme.Values.themeControlOutline
            }
        },
        State {
            name: "press"
            when: myButton.hover && myButton.pressed && myButton.enabled
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeInteraction
                border.color: StudioTheme.Values.themeInteraction
            }
            PropertyChanges {
                target: myButton
                z: 100
            }
        },
        State {
            name: "check"
            when: myButton.enabled && !myButton.pressed && myButton.checked
            PropertyChanges {
                target: buttonBackground
                color: myButton.checkedInverted ? StudioTheme.Values.themeInteraction
                                                : StudioTheme.Values.themeControlBackground
                border.color: myButton.checkedInverted ? StudioTheme.Values.themeInteraction
                                                       : StudioTheme.Values.themeControlOutline
            }
        },
        State {
            name: "disable"
            when: !myButton.enabled
            PropertyChanges {
                target: buttonBackground
                color: StudioTheme.Values.themeControlBackgroundDisabled
                border.color: StudioTheme.Values.themeControlOutlineDisabled
            }
        }
    ]
}
