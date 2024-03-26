// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15
import LandingPage as Theme

Button {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    leftPadding: 4
    rightPadding: 4
    hoverEnabled: true
    font.family: Theme.Values.baseFont
    font.pixelSize: 16

    background: Rectangle {
        id: buttonBackground
        color: Theme.Colors.backgroundPrimary
        implicitWidth: 100
        implicitHeight: 35
        border.color: Theme.Colors.foregroundPrimary
        anchors.fill: parent
    }

    contentItem: Text {
        id: textItem
        text: control.text
        font: control.font
        color: Theme.Colors.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        rightPadding: 5
        leftPadding: 5
    }

    states: [
        State {
            name: "default"
            when: control.enabled && !control.hovered && !control.pressed && !control.checked
            PropertyChanges {
                target: buttonBackground
                color: Theme.Colors.backgroundPrimary
            }
            PropertyChanges {
                target: textItem
                color: Theme.Colors.text
            }
        },
        State {
            name: "hover"
            extend: "default"
            when: control.enabled && control.hovered && !control.pressed
            PropertyChanges {
                target: buttonBackground
                color: Theme.Colors.hover
            }
        },
        State {
            name: "press"
            extend: "default"
            when: control.hovered && control.pressed
            PropertyChanges {
                target: buttonBackground
                color: Theme.Colors.accent
                border.color: Theme.Colors.accent
            }
            PropertyChanges {
                target: textItem
                color: Theme.Colors.backgroundPrimary
            }
        },
        State {
            name: "disable"
            when: !control.enabled
            PropertyChanges {
                target: buttonBackground
                color: Theme.Colors.backgroundPrimary
                border.color: Theme.Colors.foregroundSecondary
            }
            PropertyChanges {
                target: textItem
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

