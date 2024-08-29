// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T

import StudioTheme as StudioTheme
import HelperWidgets

T.AbstractButton {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property bool globalHover: false
    property bool hover: control.hovered
    property bool press: control.pressed

    property alias backgroundVisible: buttonBackground.visible
    property alias backgroundRadius: buttonBackground.radius

    // Inverts the checked style
    property bool checkedInverted: false

    property int warningCount: 0
    property int errorCount: 0

    property bool hasWarnings: control.warningCount > 0
    property bool hasErrors: control.errorCount > 0

    property alias tooltip: toolTipArea.tooltip

    ToolTipArea {
        id: toolTipArea
        anchors.fill: parent
        // Without setting the acceptedButtons property the clicked event won't
        // reach the AbstractButton, it will be consumed by the ToolTipArea
        acceptedButtons: Qt.NoButton
    }

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    width: control.style.squareControlSize.width
    height: control.style.squareControlSize.height
    z: control.checked ? 10 : 3
    activeFocusOnTab: false

    background: Rectangle {
        id: buttonBackground
        color: control.style.background.idle
        border.color: control.style.border.idle
        border.width: control.style.borderWidth
        radius: control.style.radius
    }

    component CustomLabel : T.Label {
        id: customLabel
        color: control.style.icon.idle
        font.pixelSize: control.style.baseIconFontSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        property bool active: false
        property color activeColor: "red"

        states: [
            State {
                name: "default"
                when: control.enabled && !control.press && !control.checked && !control.hover
                PropertyChanges {
                    target: customLabel
                    color: customLabel.active ? customLabel.activeColor : control.style.icon.idle
                }
            },
            State {
                name: "hover"
                when: control.enabled && !control.press && !control.checked && control.hover
                PropertyChanges {
                    target: customLabel
                    color: customLabel.active ? customLabel.activeColor : control.style.icon.hover
                }
            },
            State {
                name: "press"
                when: control.enabled && control.press
                PropertyChanges {
                    target: customLabel
                    color: control.style.icon.interaction
                }
            },
            State {
                name: "check"
                when: control.enabled && !control.press && control.checked
                PropertyChanges {
                    target: customLabel
                    color: control.checkedInverted ? control.style.text.selectedText // TODO rather something in icon
                                                   : control.style.icon.selected
                }
            },
            State {
                name: "disable"
                when: !control.enabled
                PropertyChanges {
                    target: customLabel
                    color: control.style.icon.disabled
                }
            }
        ]
    }

    indicator: Item {
        x: 0
        y: 0
        width: control.width
        height: control.height

        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 8

            CustomLabel {
                height: control.height
                font.pixelSize: StudioTheme.Values.baseFontSize
                text: control.warningCount
                active: control.hasWarnings
                activeColor: StudioTheme.Values.themeAmberLight
            }

            CustomLabel {
                height: control.height
                text: StudioTheme.Constants.warning2_medium
                font.family: StudioTheme.Constants.iconFont.family
                active: control.hasWarnings
                activeColor: StudioTheme.Values.themeAmberLight
            }

            CustomLabel {
                height: control.height
                text: StudioTheme.Constants.error_medium
                font.family: StudioTheme.Constants.iconFont.family
                active: control.hasErrors
                activeColor: StudioTheme.Values.themeRedLight
            }

            CustomLabel {
                height: control.height
                font.pixelSize: StudioTheme.Values.baseFontSize
                text: control.errorCount
                active: control.hasErrors
                activeColor: StudioTheme.Values.themeRedLight
            }
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
