// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

T.RadioButton {
    id: root

    property alias actionIndicator: actionIndicator

    // This property is used to indicate the global hover state
    property bool hover: root.hovered && root.enabled
    property bool edit: false

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: StudioTheme.Values.actionIndicatorWidth
    property real __actionIndicatorHeight: StudioTheme.Values.actionIndicatorHeight

    property alias labelVisible: radioButtonLabel.visible
    property alias labelColor: radioButtonLabel.color

    property alias fontFamily: radioButtonLabel.font.family
    property alias fontPixelSize: radioButtonLabel.font.pixelSize

    font.pixelSize: StudioTheme.Values.myFontSize

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    spacing: StudioTheme.Values.radioButtonSpacing
    hoverEnabled: true
    activeFocusOnTab: false

    ActionIndicator {
        id: actionIndicator
        myControl: root
        width: actionIndicator.visible ? root.__actionIndicatorWidth : 0
        height: actionIndicator.visible ? root.__actionIndicatorHeight : 0
    }

    indicator: Rectangle {
        id: radioButtonBackground
        implicitWidth: StudioTheme.Values.radioButtonWidth
        implicitHeight: StudioTheme.Values.radioButtonHeight

        x: actionIndicator.width
        y: 0
        z: 5

        radius: width / 2
        color: StudioTheme.Values.themeControlBackground
        border.color: StudioTheme.Values.themeControlOutline
        border.width: StudioTheme.Values.border

        Rectangle {
            id: radioButtonIndicator
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: StudioTheme.Values.radioButtonIndicatorWidth
            height: StudioTheme.Values.radioButtonIndicatorHeight
            radius: width / 2
            color: StudioTheme.Values.themeInteraction
            visible: root.checked
        }
    }

    contentItem: T.Label {
        id: radioButtonLabel
        leftPadding: radioButtonBackground.x + radioButtonBackground.width + root.spacing
        rightPadding: 0

        verticalAlignment: Text.AlignVCenter
        text: root.text
        font: root.font
        color: StudioTheme.Values.themeTextColor
        visible: text !== ""
    }

    states: [
        State {
            name: "default"
            when: root.enabled && !root.hover && !root.pressed && !actionIndicator.hover
            PropertyChanges {
                target: radioButtonBackground
                color: StudioTheme.Values.themeControlBackground
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: radioButtonIndicator
                color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "globalHover"
            when: actionIndicator.hover && !root.pressed && root.enabled
            PropertyChanges {
                target: radioButtonBackground
                color: StudioTheme.Values.themeControlBackgroundGlobalHover
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: radioButtonIndicator
                color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "hover"
            when: root.hover && !actionIndicator.hover && !root.pressed
            PropertyChanges {
                target: radioButtonBackground
                color: StudioTheme.Values.themeControlBackgroundHover
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: radioButtonIndicator
                color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "press"
            when: root.hover && root.pressed
            PropertyChanges {
                target: radioButtonBackground
                color: StudioTheme.Values.themeControlBackgroundInteraction
                border.color: StudioTheme.Values.themeControlOutlineInteraction
            }
            PropertyChanges {
                target: radioButtonIndicator
                color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "disable"
            when: !root.enabled
            PropertyChanges {
                target: radioButtonBackground
                color: StudioTheme.Values.themeControlBackgroundDisabled
                border.color: StudioTheme.Values.themeControlOutlineDisabled
            }
            PropertyChanges {
                target: radioButtonIndicator
                color: StudioTheme.Values.themeIconColorDisabled
            }
        }
    ]
}
