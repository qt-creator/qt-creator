// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15
import LandingPage as Theme

CheckBox {
    id: control
    autoExclusive: false

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    spacing: Theme.Values.checkBoxSpacing
    hoverEnabled: true
    font.family: Theme.Values.baseFont

    indicator: Rectangle {
        id: checkBoxBackground
        implicitWidth: Theme.Values.checkBoxSize
        implicitHeight: Theme.Values.checkBoxSize
        x: 0
        y: parent.height / 2 - height / 2
        color: Theme.Colors.backgroundPrimary
        border.color: Theme.Colors.foregroundSecondary
        border.width: Theme.Values.border

        Rectangle {
            id: checkBoxIndicator
            width: Theme.Values.checkBoxIndicatorSize
            height: Theme.Values.checkBoxIndicatorSize
            x: (Theme.Values.checkBoxSize - Theme.Values.checkBoxIndicatorSize) * 0.5
            y: (Theme.Values.checkBoxSize - Theme.Values.checkBoxIndicatorSize) * 0.5
            color: Theme.Colors.accent
            visible: control.checked
        }
    }

    contentItem: Text {
        id: checkBoxLabel
        text: control.text
        font: control.font
        color: Theme.Colors.text
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
    }

    states: [
        State {
            name: "default"
            when: control.enabled && !control.hovered && !control.pressed
            PropertyChanges {
                target: checkBoxBackground
                color: Theme.Colors.backgroundPrimary
                border.color: Theme.Colors.foregroundSecondary
            }
            PropertyChanges {
                target: checkBoxLabel
                color: Theme.Colors.text
            }
        },
        State {
            name: "hover"
            when: control.enabled && control.hovered && !control.pressed
            PropertyChanges {
                target: checkBoxBackground
                color: Theme.Colors.hover
                border.color: Theme.Colors.foregroundSecondary
            }
        },
        State {
            name: "press"
            when: control.hovered && control.pressed
            PropertyChanges {
                target: checkBoxBackground
                color: Theme.Colors.hover
                border.color: Theme.Colors.accent
            }
            PropertyChanges {
                target: checkBoxIndicator
                color: Theme.Colors.backgroundSecondary
            }
        },
        State {
            name: "disable"
            when: !control.enabled
            PropertyChanges {
                target: checkBoxBackground
                color: Theme.Colors.backgroundPrimary
                border.color: Theme.Colors.disabledLink
            }
            PropertyChanges {
                target: checkBoxIndicator
                color: Theme.Colors.disabledLink
            }
            PropertyChanges {
                target: checkBoxLabel
                color: Theme.Colors.disabledLink
            }
        }
    ]
}

/*##^##
Designer {
    D{i:0;height:40;width:142}
}
##^##*/

