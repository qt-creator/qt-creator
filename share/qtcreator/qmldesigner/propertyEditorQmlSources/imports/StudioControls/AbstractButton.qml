// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.AbstractButton {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property bool globalHover: false
    property bool hover: control.hovered
    property bool press: control.pressed

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
    width: control.style.squareControlSize.width
    height: control.style.squareControlSize.height
    z: control.checked ? 10 : 3
    activeFocusOnTab: false

    onHoverChanged: {
        if (parent !== undefined && parent.hoverCallback !== undefined && control.enabled)
            parent.hoverCallback()
    }

    background: Rectangle {
        id: buttonBackground
        color: control.style.background.idle
        border.color: control.style.border.idle
        border.width: control.style.borderWidth
        radius: control.style.radius
    }

    indicator: Item {
        x: 0
        y: 0
        width: control.width
        height: control.height

        T.Label {
            id: buttonIcon
            color: control.style.icon.idle
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: control.style.baseIconFontSize
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            anchors.fill: parent
            renderType: Text.QtRendering

            states: [
                State {
                    name: "default"
                    when: control.enabled && !control.press && !control.checked && !control.hover
                    PropertyChanges {
                        target: buttonIcon
                        color: control.style.icon.idle
                    }
                },
                State {
                    name: "hover"
                    when: control.enabled && !control.press && !control.checked && control.hover
                    PropertyChanges {
                        target: buttonIcon
                        color: control.style.icon.hover
                    }
                },
                State {
                    name: "press"
                    when: control.enabled && control.press
                    PropertyChanges {
                        target: buttonIcon
                        color: control.style.icon.interaction
                    }
                },
                State {
                    name: "check"
                    when: control.enabled && !control.press && control.checked
                    PropertyChanges {
                        target: buttonIcon
                        color: control.checkedInverted ? control.style.text.selectedText // TODO rather something in icon
                                                       : control.style.icon.selected
                    }
                },
                State {
                    name: "disable"
                    when: !control.enabled
                    PropertyChanges {
                        target: buttonIcon
                        color: control.style.icon.disabled
                    }
                }
            ]
        }
    }

    states: [
        State {
            name: "default"
            when: control.enabled && !control.globalHover && !control.hover
                  && !control.press && !control.checked
            PropertyChanges {
                target: buttonBackground
                color: control.style.background.idle
                border.color: control.style.border.idle
            }
            PropertyChanges {
                target: control
                z: 3
            }
        },
        State {
            name: "globalHover"
            when: control.globalHover && !control.hover && !control.press && control.enabled
            PropertyChanges {
                target: buttonBackground
                color: control.style.background.idle
                border.color: control.style.border.idle
            }
        },
        State {
            name: "hover"
            when: !control.checked && control.hover && !control.press && control.enabled
            PropertyChanges {
                target: buttonBackground
                color: control.style.background.hover
                border.color: control.style.border.hover
            }
            PropertyChanges {
                target: control
                z: 100
            }
        },
        State {
            name: "hoverCheck"
            when: control.checked && control.hover && !control.press && control.enabled
            PropertyChanges {
                target: buttonBackground
                color: control.checkedInverted ? control.style.interactionHover
                                               : control.style.background.hover
                border.color: control.checkedInverted ? control.style.interactionHover
                                                      : control.style.border.hover
            }
            PropertyChanges {
                target: control
                z: 100
            }
        },
        State {
            name: "press"
            when: control.hover && control.press && control.enabled
            PropertyChanges {
                target: buttonBackground
                color: control.style.interaction
                border.color: control.style.interaction
            }
            PropertyChanges {
                target: control
                z: 100
            }
        },
        State {
            name: "check"
            when: control.enabled && !control.press && control.checked
            PropertyChanges {
                target: buttonBackground
                color: control.checkedInverted ? control.style.interaction
                                               : control.style.background.idle
                border.color: control.checkedInverted ? control.style.interaction
                                                      : control.style.border.idle
            }
        },
        State {
            name: "disable"
            when: !control.enabled
            PropertyChanges {
                target: buttonBackground
                color: control.style.background.disabled
                border.color: control.style.border.disabled
            }
        }
    ]
}
