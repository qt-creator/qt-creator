// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T

import StudioControls as StudioControls
import StudioTheme as StudioTheme

T.TextField {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    signal buttonClicked

    property bool empty: control.text === ""

    property alias buttonIcon: buttonIcon.text

    function isEmpty() {
        return control.text === ""
    }

    property string tooltip

    StudioControls.ToolTipExt { id: toolTip }

    HoverHandler { id: hoverHandler }

    Timer {
        interval: 1000
        running: control.hovered && control.tooltip.length
        onTriggered: toolTip.showText(control, Qt.point(hoverHandler.point.position.x,
                                                        hoverHandler.point.position.y),
                                      control.tooltip)
    }

    width: control.style.controlSize.width
    height: control.style.controlSize.height

    horizontalAlignment: Qt.AlignLeft
    verticalAlignment: Qt.AlignVCenter

    leftPadding: 32
    rightPadding: 30

    font.pixelSize: control.style.baseFontSize

    color: control.style.text.idle
    selectionColor: control.style.text.selection
    selectedTextColor: control.style.text.selectedText
    placeholderTextColor: control.style.text.placeholder

    selectByMouse: true
    readOnly: false
    hoverEnabled: true
    clip: true

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape && event.modifiers === Qt.NoModifier) {
            control.clear()
            event.accepted = true
        }
    }

    Text {
        id: placeholder
        x: control.leftPadding
        y: control.topPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        height: control.height - (control.topPadding + control.bottomPadding)

        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        verticalAlignment: control.verticalAlignment
        visible: !control.length && !control.preeditText
                 && (!control.activeFocus || control.horizontalAlignment !== Qt.AlignHCenter)
        elide: Text.ElideRight
        renderType: control.renderType
    }

    background: Rectangle {
        id: textFieldBackground
        color: control.style.background.idle
        //border.color: control.style.border.idle
        border.width: 0//control.style.borderWidth
        radius: control.style.radius
    }

    T.AbstractButton {
        id: button

        property StudioTheme.ControlStyle style: StudioTheme.Values.viewBarButtonStyle

        width: button.style.squareControlSize.width
        height: button.style.squareControlSize.height

        anchors.left: parent.left

        enabled: control.acceptableInput

        onClicked: control.buttonClicked()

        contentItem: T.Label {
            id: buttonIcon
            width: button.style.squareControlSize.width
            height: button.height
            font {
                family: StudioTheme.Constants.iconFont.family
                pixelSize: button.style.baseIconFontSize
            }
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            id: buttonBackground
            color: button.style.background.idle
            border.color: "transparent"//button.style.border.idle
            border.width: button.style.borderWidth
            radius: button.style.radius
            topRightRadius: 0
            bottomRightRadius: 0
        }

        states: [
            State {
                name: "default"
                when: button.enabled && !button.hovered && !button.pressed && !button.checked
                PropertyChanges {
                    target: buttonBackground
                    color: button.style.background.idle
                    border.color: button.style.border.idle
                }
                PropertyChanges {
                    target: buttonIcon
                    color: button.style.icon.idle
                }
            },
            State {
                name: "hover"
                when: button.enabled && button.hovered && !button.pressed && !button.checked
                PropertyChanges {
                    target: buttonBackground
                    color: button.style.background.hover
                    border.color: button.style.border.hover
                }
                PropertyChanges {
                    target: buttonIcon
                    color: button.style.icon.hover
                }
            },
            State {
                name: "press"
                when: button.enabled && button.hovered && button.pressed
                PropertyChanges {
                    target: buttonBackground
                    color: button.style.interaction
                    border.color: button.style.interaction
                }
                PropertyChanges {
                    target: buttonIcon
                    color: button.style.icon.interaction
                }
            },
            State {
                name: "pressNotHover"
                when: button.enabled && !button.hovered && button.pressed
                extend: "hover"
            },
            State {
                name: "disable"
                when: !button.enabled
                PropertyChanges {
                    target: buttonBackground
                    color: button.style.background.disabled
                    border.color: button.style.border.disabled
                }
                PropertyChanges {
                    target: buttonIcon
                    color: button.style.icon.disabled
                }
            }
        ]
    }

    Rectangle {
        id: controlBorder
        anchors.fill: parent
        color: "transparent"
        border.color: control.style.border.idle
        border.width: control.style.borderWidth
        radius: control.style.radius
    }

    Rectangle { // x button
        width: 16
        height: 16
        anchors.right: parent.right
        anchors.rightMargin: 5
        anchors.verticalCenter: parent.verticalCenter
        visible: control.text !== ""
        color: "transparent"

        T.Label {
            text: StudioTheme.Constants.close_small
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: control.style.baseIconFontSize
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            anchors.centerIn: parent
            color: control.style.icon.idle

            scale: xMouseArea.containsMouse ? 1.2 : 1
        }

        MouseArea {
            id: xMouseArea
            hoverEnabled: true
            anchors.fill: parent
            onClicked: control.text = ""
        }
    }

    states: [
        State {
            name: "default"
            when: control.enabled && !control.hovered && !control.activeFocus
            PropertyChanges {
                target: textFieldBackground
                color: control.style.background.idle
            }
            PropertyChanges {
                target: controlBorder
                border.color: control.style.border.idle
            }

            PropertyChanges {
                target: control
                placeholderTextColor: control.style.text.placeholder
            }
        },
        State {
            name: "hover"
            when: control.enabled && control.hovered && !control.activeFocus
            PropertyChanges {
                target: textFieldBackground
                color: control.style.background.hover
            }
            PropertyChanges {
                target: controlBorder
                border.color: control.style.border.hover
            }
            PropertyChanges {
                target: control
                placeholderTextColor: control.style.text.placeholderHover
            }
        },
        State {
            name: "edit"
            when: control.enabled && control.activeFocus
            PropertyChanges {
                target: textFieldBackground
                color: control.style.background.interaction
            }
            PropertyChanges {
                target: controlBorder
                border.color: control.style.border.interaction
            }
            PropertyChanges {
                target: control
                placeholderTextColor: control.style.text.placeholderInteraction
            }
        },
        State {
            name: "disabled"
            when: !control.enabled
            PropertyChanges {
                target: control
                placeholderTextColor: control.style.text.disabled
            }
        }
    ]
}
