// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

T.Switch {
    id: root

    property alias actionIndicator: actionIndicator

    // This property is used to indicate the global hover state
    property bool hover: root.hovered && root.enabled
    property bool edit: false

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: StudioTheme.Values.actionIndicatorWidth
    property real __actionIndicatorHeight: StudioTheme.Values.actionIndicatorHeight

    property alias labelVisible: switchLabel.visible
    property alias labelColor: switchLabel.color

    property alias fontFamily: switchLabel.font.family
    property alias fontPixelSize: switchLabel.font.pixelSize

    font.pixelSize: StudioTheme.Values.myFontSize

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    spacing: StudioTheme.Values.switchSpacing
    hoverEnabled: true
    activeFocusOnTab: false

    ActionIndicator {
        id: actionIndicator
        myControl: root
        width: actionIndicator.visible ? root.__actionIndicatorWidth : 0
        height: actionIndicator.visible ? root.__actionIndicatorHeight : 0
    }

    indicator: Rectangle {
        id: switchBackground
        x: actionIndicator.width
        y: 0
        z: 5
        implicitWidth: StudioTheme.Values.height * 2
        implicitHeight: StudioTheme.Values.height
        radius: StudioTheme.Values.height * 0.5
        color: StudioTheme.Values.themeControlBackground
        border.width: StudioTheme.Values.border
        border.color: StudioTheme.Values.themeControlOutline

        Rectangle {
            id: switchIndicator

            readonly property real gap: 5
            property real size: switchBackground.implicitHeight - switchIndicator.gap * 2

            x: root.checked ? parent.width - width - switchIndicator.gap
                            : switchIndicator.gap
            y: switchIndicator.gap
            width: switchIndicator.size
            height: switchIndicator.size
            radius: switchIndicator.size * 0.5
            color: StudioTheme.Values.themeTextColor
            border.width: 0
        }
    }

    contentItem: T.Label {
        id: switchLabel
        leftPadding: switchBackground.x + switchBackground.width + root.spacing
        rightPadding: 0
        verticalAlignment: Text.AlignVCenter
        text: root.text
        font: root.font
        color: StudioTheme.Values.themeTextColor
        visible: text !== ""
    }

    property bool __default: root.enabled && !root.hover && !actionIndicator.hover && !root.pressed
    property bool __globalHover: root.enabled && actionIndicator.hover && !root.pressed
    property bool __hover: root.hover && !actionIndicator.hover && !root.pressed
    property bool __press: root.hover && root.pressed

    states: [
        State {
            name: "default"
            when: root.__default && !root.checked
            PropertyChanges {
                target: switchBackground
                color: StudioTheme.Values.themeControlBackground
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: switchIndicator
                color: StudioTheme.Values.themeTextColor
            }
        },
        State {
            name: "globalHover"
            when: root.__globalHover && !root.checked
            PropertyChanges {
                target: switchBackground
                color: StudioTheme.Values.themeControlBackgroundGlobalHover
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: switchIndicator
                color: StudioTheme.Values.themeTextColor
            }
        },
        State {
            name: "hover"
            when: root.__hover && !root.checked
            PropertyChanges {
                target: switchBackground
                color: StudioTheme.Values.themeControlBackgroundHover
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: switchIndicator
                color: StudioTheme.Values.themeTextColor
            }
        },
        State {
            name: "press"
            when: root.__press && !root.checked
            PropertyChanges {
                target: switchBackground
                color: StudioTheme.Values.themeControlBackgroundInteraction
                border.color: StudioTheme.Values.themeControlOutlineInteraction
            }
            PropertyChanges {
                target: switchIndicator
                color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "disable"
            when: !root.enabled && !root.checked
            PropertyChanges {
                target: switchBackground
                color: StudioTheme.Values.themeControlBackgroundDisabled
                border.color: StudioTheme.Values.themeControlOutlineDisabled
            }
            PropertyChanges {
                target: switchIndicator
                color: StudioTheme.Values.themeTextColorDisabled
            }
            PropertyChanges {
                target: switchLabel
                color: StudioTheme.Values.themeTextColorDisabled
            }
        },

        State {
            name: "defaultChecked"
            when: root.__default && root.checked
            extend: "default"
            PropertyChanges {
                target: switchBackground
                color: StudioTheme.Values.themeInteraction
                border.color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "globalHoverChecked"
            when: root.__globalHover && root.checked
            extend: "globalHover"
            PropertyChanges {
                target: switchBackground
                color: StudioTheme.Values.themeInteractionHover
                border.color: StudioTheme.Values.themeInteractionHover
            }
        },
        State {
            name: "hoverChecked"
            when: root.__hover && root.checked
            extend: "hover"
            PropertyChanges {
                target: switchBackground
                color: StudioTheme.Values.themeInteractionHover
                border.color: StudioTheme.Values.themeInteractionHover
            }
        },
        State {
            name: "pressChecked"
            when: root.__press && root.checked
            extend: "press"
        },
        State {
            name: "disableChecked"
            when: !root.enabled && root.checked
            extend: "disable"
        }
    ]
}
