// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T

import HelperWidgets 2.0 as HelperWidgets
import StudioTheme as StudioTheme

T.AbstractButton {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.viewBarButtonStyle

    property alias tooltip: toolTipArea.tooltip
    property alias buttonIcon: buttonIcon.text

    width: control.style.squareControlSize.width + buttonLabel.implicitWidth
    height: control.style.squareControlSize.height

    HelperWidgets.ToolTipArea {
        id: toolTipArea
        anchors.fill: parent
        // Without setting the acceptedButtons property the clicked event won't
        // reach the AbstractButton, it will be consumed by the ToolTipArea
        acceptedButtons: Qt.NoButton
    }

    contentItem: Row {
        spacing: 0

        T.Label {
            id: buttonIcon
            width: control.style.squareControlSize.width
            height: control.height
            font {
                family: StudioTheme.Constants.iconFont.family
                pixelSize: control.style.baseIconFontSize
            }
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        T.Label {
            id: buttonLabel
            height: control.height
            rightPadding: buttonLabel.visible ? 8 : 0
            font.pixelSize: control.style.baseFontSize
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: control.text
            visible: control.text !== ""
        }
    }

    background: Rectangle {
        id: controlBackground
        color: control.style.background.idle
        border.color: control.style.border.idle
        border.width: control.style.borderWidth
        radius: control.style.radius
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
            name: "press"
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
            name: "pressNotHover"
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
