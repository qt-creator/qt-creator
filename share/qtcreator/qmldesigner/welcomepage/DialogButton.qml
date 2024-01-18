// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates
import WelcomeScreen 1.0

Button {
    id: root

    implicitWidth: Math.max(
                       background ? background.implicitWidth : 0,
                       textItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(
                        background ? background.implicitHeight : 0,
                        textItem.implicitHeight + topPadding + bottomPadding)
    leftPadding: 4
    rightPadding: 4


    background: Rectangle {
        id: background
        implicitWidth: 80
        implicitHeight: 20
        opacity: enabled ? 1 : 0.3
        radius: 2
        color: Constants.currentPushButtonNormalBackground
        border.color: Constants.currentPushButtonNormalOutline
        anchors.fill: parent
    }

    contentItem: Text {
        id: textItem
        text: root.text
        font.pixelSize: 12
        opacity: enabled ? 1.0 : 0.3
        color: Constants.currentGlobalText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    states: [
        State {
            name: "default"
            when: !root.down && !root.hovered && !root.checked

            PropertyChanges {
                target: background
                color: Constants.currentPushButtonNormalBackground
                border.color: Constants.currentPushButtonNormalOutline
            }

            PropertyChanges {
                target: textItem
                color: Constants.currentGlobalText
            }
        },
        State {
            name: "hover"
            when: root.hovered && !root.checked && !root.down

            PropertyChanges {
                target: textItem
                color: Constants.currentGlobalText
            }

            PropertyChanges {
                target: background
                color: Constants.currentPushButtonHoverBackground
                border.color: Constants.currentPushButtonHoverOutline
            }
        },
        State {
            name: "active"
            when: root.checked || root.down

            PropertyChanges {
                target: textItem
                color: Constants.currentActiveGlobalText
            }

            PropertyChanges {
                target: background
                color: Constants.currentBrand
                border.color: "#00000000"
            }
        }
    ]
}
