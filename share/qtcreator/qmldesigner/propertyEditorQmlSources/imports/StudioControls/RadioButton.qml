// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.RadioButton {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property alias actionIndicator: actionIndicator

    // This property is used to indicate the global hover state
    property bool hover: control.hovered && control.enabled
    property bool edit: false

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: control.style.actionIndicatorSize.width
    property real __actionIndicatorHeight: control.style.actionIndicatorSize.height

    property alias labelVisible: radioButtonLabel.visible
    property alias labelColor: radioButtonLabel.color

    property alias fontFamily: radioButtonLabel.font.family
    property alias fontPixelSize: radioButtonLabel.font.pixelSize

    font.pixelSize: control.style.baseFontSize

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    spacing: control.style.controlSpacing
    hoverEnabled: true
    activeFocusOnTab: false

    ActionIndicator {
        id: actionIndicator
        style: control.style
        __parentControl: control
        width: actionIndicator.visible ? control.__actionIndicatorWidth : 0
        height: actionIndicator.visible ? control.__actionIndicatorHeight : 0
    }

    indicator: Rectangle {
        id: radioButtonBackground
        implicitWidth: control.style.squareControlSize.width
        implicitHeight: control.style.squareControlSize.height

        x: actionIndicator.width
        y: 0
        z: 5

        radius: width / 2
        color: control.style.background.idle
        border.color: control.style.border.idle
        border.width: control.style.borderWidth

        Rectangle {
            id: radioButtonIndicator
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: control.style.radioButtonIndicatorSize.width
            height: control.style.radioButtonIndicatorSize.height
            radius: width / 2
            color: control.style.interaction
            visible: control.checked
        }
    }

    contentItem: T.Label {
        id: radioButtonLabel
        leftPadding: radioButtonBackground.x + radioButtonBackground.width + control.spacing
        rightPadding: 0

        verticalAlignment: Text.AlignVCenter
        text: control.text
        font: control.font
        color: control.style.text.idle
        visible: text !== ""
    }

    states: [
        State {
            name: "default"
            when: control.enabled && !control.hover && !control.pressed && !actionIndicator.hover
            PropertyChanges {
                target: radioButtonBackground
                color: control.style.background.idle
                border.color: control.style.border.idle
            }
            PropertyChanges {
                target: radioButtonIndicator
                color: control.style.interaction
            }
        },
        State {
            name: "globalHover"
            when: actionIndicator.hover && !control.pressed && control.enabled
            PropertyChanges {
                target: radioButtonBackground
                color: control.style.background.globalHover
                border.color: control.style.border.idle
            }
            PropertyChanges {
                target: radioButtonIndicator
                color: control.style.interaction
            }
        },
        State {
            name: "hover"
            when: control.hover && !actionIndicator.hover && !control.pressed
            PropertyChanges {
                target: radioButtonBackground
                color: control.style.background.hover
                border.color: control.style.border.hover
            }
            PropertyChanges {
                target: radioButtonIndicator
                color: control.style.interaction
            }
        },
        State {
            name: "press"
            when: control.hover && control.pressed
            PropertyChanges {
                target: radioButtonBackground
                color: control.style.background.interaction
                border.color: control.style.border.interaction
            }
            PropertyChanges {
                target: radioButtonIndicator
                color: control.style.interaction
            }
        },
        State {
            name: "disable"
            when: !control.enabled
            PropertyChanges {
                target: radioButtonBackground
                color: control.style.background.disabled
                border.color: control.style.border.disabled
            }
            PropertyChanges {
                target: radioButtonIndicator
                color: control.style.icon.disabled
            }
        }
    ]
}
