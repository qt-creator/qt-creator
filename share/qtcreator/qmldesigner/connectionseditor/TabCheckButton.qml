// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme as StudioTheme

T.TabButton {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.viewBarButtonStyle

    property alias tooltip: toolTipArea.tooltip
    property alias buttonIcon: buttonIcon.text

    width: control.style.squareControlSize.width + buttonLabel.implicitWidth
           + buttonLabel.leftPadding + buttonLabel.rightPadding
    height: control.style.squareControlSize.height

    contentItem: Row {
        spacing: 0

        Text {
            id: buttonIcon
            width: control.style.squareControlSize.width
            height: control.height
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: control.style.baseIconFontSize
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Text {
            id: buttonLabel
            height: control.height
            rightPadding: 4
            font.pixelSize: control.style.baseFontSize
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: control.text
        }
    }

    background: Rectangle {
        id: controlBackground
        color: control.style.background.idle
        border.color: control.style.border.idle
        border.width: control.style.borderWidth
        radius: StudioTheme.Values.smallRadius //control.style.radius
    }

    HelperWidgets.ToolTipArea {
        id: toolTipArea
        anchors.fill: parent
        // Without setting the acceptedButtons property the clicked event won't
        // reach the AbstractButton, it will be consumed by the ToolTipArea
        acceptedButtons: Qt.NoButton
    }

    states: [
        State {
            name: "default"
            when: control.enabled && !control.hovered && !control.pressed && !control.checked
            PropertyChanges {
                target: controlBackground
                color: control.style.background.idle
                border.color: control.style.border.idle
            }
            PropertyChanges {
                target: buttonIcon
                color: control.style.icon.idle
            }
            PropertyChanges {
                target: buttonLabel
                color: control.style.icon.idle
            }
        },
        State {
            name: "hover"
            when: control.enabled && control.hovered && !control.pressed && !control.checked
            PropertyChanges {
                target: controlBackground
                color: control.style.background.hover
                border.color: control.style.border.hover
            }
            PropertyChanges {
                target: buttonIcon
                color: control.style.icon.hover
            }
            PropertyChanges {
                target: buttonLabel
                color: control.style.icon.hover
            }
        },
        State {
            name: "hoverCheck"
            when: control.enabled && control.hovered && !control.pressed && control.checked
            PropertyChanges {
                target: controlBackground
                color: control.style.interactionHover
                border.color: control.style.interactionHover
            }
            PropertyChanges {
                target: buttonIcon
                color: control.style.text.selectedText
            }
            PropertyChanges {
                target: buttonLabel
                color: control.style.text.selectedText
            }
        },
        State {
            name: "pressed"
            when: control.enabled && control.hovered && control.pressed
            PropertyChanges {
                target: controlBackground
                color: control.style.interaction
                border.color: control.style.interaction
            }
            PropertyChanges {
                target: buttonIcon
                color: control.style.icon.interaction
            }
            PropertyChanges {
                target: buttonLabel
                color: control.style.icon.interaction
            }
        },
        State {
            name: "check"
            when: control.enabled && !control.pressed && control.checked
            extend: "hoverCheck"
            PropertyChanges {
                target: controlBackground
                color: control.style.interaction
                border.color: control.style.interaction
            }
        },
        State {
            name: "pressedButNotHovered"
            when: control.enabled && !control.hovered && control.pressed
            extend: "hover"
        },
        State {
            name: "disable"
            when: !control.enabled
            PropertyChanges {
                target: controlBackground
                color: control.style.background.disabled
                border.color: control.style.border.disabled
            }
            PropertyChanges {
                target: buttonIcon
                color: control.style.icon.disabled
            }
            PropertyChanges {
                target: buttonLabel
                color: control.style.icon.disabled
            }
        }
    ]
}
