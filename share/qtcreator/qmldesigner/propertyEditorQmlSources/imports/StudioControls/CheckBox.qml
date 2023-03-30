// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.CheckBox {
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

    font.pixelSize: control.style.baseFontSize

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
        id: checkBoxBackground
        x: actionIndicator.width
        y: 0
        z: 5
        implicitWidth: control.style.squareControlSize.width
        implicitHeight: control.style.squareControlSize.height
        color: control.style.background.idle
        border.color: control.style.border.idle
        border.width: control.style.borderWidth

        T.Label {
            id: checkedIcon
            x: (parent.width - checkedIcon.width) / 2
            y: (parent.height - checkedIcon.height) / 2
            text: StudioTheme.Constants.tickIcon
            visible: control.checkState === Qt.Checked
            color: control.style.icon.idle
            font.pixelSize: control.style.baseIconFontSize
            font.family: StudioTheme.Constants.iconFont.family
        }

        T.Label {
            id: partiallyCheckedIcon
            x: (parent.width - checkedIcon.width) / 2
            y: (parent.height - checkedIcon.height) / 2
            text: StudioTheme.Constants.triState
            visible: control.checkState === Qt.PartiallyChecked
            color: control.style.icon.idle
            font.pixelSize: control.style.baseIconFontSize
            font.family: StudioTheme.Constants.iconFont.family
        }
    }

    contentItem: T.Label {
        id: label
        leftPadding: checkBoxBackground.x + checkBoxBackground.width + control.spacing
        rightPadding: 0
        verticalAlignment: Text.AlignVCenter
        text: control.text
        font: control.font
        color: control.style.text.idle
        visible: label.text !== ""
    }

    states: [
        State {
            name: "default"
            when: control.enabled && !control.hover
                  && !control.pressed && !actionIndicator.hover
            PropertyChanges {
                target: checkBoxBackground
                color: control.style.background.idle
                border.color: control.style.border.idle
            }
            PropertyChanges {
                target: checkedIcon
                color: control.style.icon.idle
            }
            PropertyChanges {
                target: partiallyCheckedIcon
                color: control.style.icon.idle
            }
        },
        State {
            name: "globalHover"
            when: actionIndicator.hover && !control.pressed && control.enabled
            PropertyChanges {
                target: checkBoxBackground
                color: control.style.background.globalHover
                border.color: control.style.border.idle
            }
            PropertyChanges {
                target: checkedIcon
                color: control.style.icon.idle
            }
            PropertyChanges {
                target: partiallyCheckedIcon
                color: control.style.icon.idle
            }
        },
        State {
            name: "hover"
            when: control.hover && !actionIndicator.hover && !control.pressed
            PropertyChanges {
                target: checkBoxBackground
                color: control.style.background.hover
                border.color: control.style.border.hover
            }
            PropertyChanges {
                target: checkedIcon
                color: control.style.icon.hover
            }
            PropertyChanges {
                target: partiallyCheckedIcon
                color: control.style.icon.hover
            }
        },
        State {
            name: "press"
            when: control.hover && control.pressed
            PropertyChanges {
                target: checkBoxBackground
                color: control.style.background.interaction
                border.color: control.style.border.interaction
            }
            PropertyChanges {
                target: checkedIcon
                color: control.style.icon.interaction
            }
            PropertyChanges {
                target: partiallyCheckedIcon
                color: control.style.icon.interaction
            }
        },
        State {
            name: "disable"
            when: !control.enabled
            PropertyChanges {
                target: checkBoxBackground
                color: control.style.background.disabled
                border.color: control.style.border.disabled
            }
            PropertyChanges {
                target: checkedIcon
                color: control.style.icon.disabled
            }
            PropertyChanges {
                target: partiallyCheckedIcon
                color: control.style.icon.disabled
            }
            PropertyChanges {
                target: label
                color: control.style.text.disabled
            }
        }
    ]
}
