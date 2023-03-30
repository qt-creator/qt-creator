// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.Switch {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property alias actionIndicator: actionIndicator

    // This property is used to indicate the global hover state
    property bool hover: control.hovered && control.enabled
    property bool edit: false

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: control.style.actionIndicatorSize.width
    property real __actionIndicatorHeight: control.style.actionIndicatorSize.height

    property alias labelVisible: label.visible
    property alias labelColor: label.color

    property alias fontFamily: label.font.family
    property alias fontPixelSize: label.font.pixelSize

    font.pixelSize: StudioTheme.Values.myFontSize

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    spacing: label.visible ? control.style.controlSpacing : 0
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
        id: switchBackground
        x: actionIndicator.width
        y: 0
        z: 5
        implicitWidth: control.style.squareControlSize.width * 2
        implicitHeight: control.style.squareControlSize.height
        radius: control.style.squareControlSize.height * 0.5
        color: control.style.background.idle
        border.color: control.style.border.idle
        border.width: control.style.borderWidth

        Rectangle {
            id: switchIndicator

            readonly property real gap: 5
            property real size: switchBackground.implicitHeight - switchIndicator.gap * 2

            x: control.checked ? parent.width - width - switchIndicator.gap
                            : switchIndicator.gap
            y: switchIndicator.gap
            width: switchIndicator.size
            height: switchIndicator.size
            radius: switchIndicator.size * 0.5
            color: control.style.icon.idle
            border.width: 0
        }
    }

    contentItem: T.Label {
        id: label
        leftPadding: switchBackground.x + switchBackground.width + control.spacing
        rightPadding: 0
        verticalAlignment: Text.AlignVCenter
        text: control.text
        font: control.font
        color: control.style.text.idle
        visible: control.text !== ""
    }

    property bool __default: control.enabled && !control.hover && !actionIndicator.hover && !control.pressed
    property bool __globalHover: control.enabled && actionIndicator.hover && !control.pressed
    property bool __hover: control.hover && !actionIndicator.hover && !control.pressed
    property bool __press: control.hover && control.pressed

    states: [
        State {
            name: "default"
            when: control.__default && !control.checked
            PropertyChanges {
                target: switchBackground
                color: control.style.background.idle
                border.color: control.style.border.idle
            }
            PropertyChanges {
                target: switchIndicator
                color: control.style.icon.idle
            }
        },
        State {
            name: "globalHover"
            when: control.__globalHover && !control.checked
            PropertyChanges {
                target: switchBackground
                color: control.style.background.globalHover
                border.color: control.style.border.idle
            }
            PropertyChanges {
                target: switchIndicator
                color: control.style.icon.idle
            }
        },
        State {
            name: "hover"
            when: control.__hover && !control.checked
            PropertyChanges {
                target: switchBackground
                color: control.style.background.hover
                border.color: control.style.border.hover
            }
            PropertyChanges {
                target: switchIndicator
                color: control.style.icon.hover
            }
        },
        State {
            name: "press"
            when: control.__press && !control.checked
            PropertyChanges {
                target: switchBackground
                color: control.style.background.interaction
                border.color: control.style.border.interaction
            }
            PropertyChanges {
                target: switchIndicator
                color: control.style.interaction
            }
        },
        State {
            name: "disable"
            when: !control.enabled && !control.checked
            PropertyChanges {
                target: switchBackground
                color: control.style.background.disabled
                border.color: control.style.border.disabled
            }
            PropertyChanges {
                target: switchIndicator
                color: control.style.icon.disabled
            }
            PropertyChanges {
                target: label
                color: control.style.text.disabled
            }
        },

        State {
            name: "defaultChecked"
            when: control.__default && control.checked
            extend: "default"
            PropertyChanges {
                target: switchBackground
                color: control.style.interaction
                border.color: control.style.interaction
            }
        },
        State {
            name: "globalHoverChecked"
            when: control.__globalHover && control.checked
            extend: "globalHover"
            PropertyChanges {
                target: switchBackground
                color: control.style.interactionHover
                border.color: control.style.interactionHover
            }
        },
        State {
            name: "hoverChecked"
            when: control.__hover && control.checked
            extend: "hover"
            PropertyChanges {
                target: switchBackground
                color: control.style.interactionHover
                border.color: control.style.interactionHover
            }
        },
        State {
            name: "pressChecked"
            when: control.__press && control.checked
            extend: "press"
        },
        State {
            name: "disableChecked"
            when: !control.enabled && control.checked
            extend: "disable"
        }
    ]
}
