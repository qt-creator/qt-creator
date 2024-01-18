// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates
import WelcomeScreen 1.0

Button {
    id: control

    implicitWidth: Math.max(
                       buttonBackground ? buttonBackground.implicitWidth : 0,
                       textItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(
                        buttonBackground ? buttonBackground.implicitHeight : 0,
                        textItem.implicitHeight + topPadding + bottomPadding)
    leftPadding: 4
    rightPadding: 4

    text: "My Button"
    checkable: true
    state: "normal"

    property bool decorated: false

    background: Rectangle {
        id: buttonBackground
        color: "#00000000"
        implicitWidth: 100
        implicitHeight: 40
        opacity: buttonBackground.enabled ? 1 : 0.3
        radius: 2
        border.color: "#047eff"
        anchors.fill: parent
    }

    contentItem: Text {
        id: textItem
        text: control.text
        font.pixelSize: 18
        opacity: textItem.enabled ? 1.0 : 0.3
        color: "#047eff"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    Rectangle {
        id: decoration
        width: 10
        visible: control.decorated
        color: Constants.currentBrand
        border.color: Constants.currentBrand
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.rightMargin: 1
        anchors.bottomMargin: 1
        anchors.topMargin: 1
    }

    states: [
        State {
            name: "normal"
            when: !control.down && !control.hovered && !control.checked

            PropertyChanges {
                target: buttonBackground
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
            when: control.hovered && !control.checked && !control.down

            PropertyChanges {
                target: textItem
                color: Constants.currentGlobalText
            }

            PropertyChanges {
                target: buttonBackground
                color: Constants.currentPushButtonHoverBackground
                border.color: Constants.currentPushButtonHoverOutline
            }
        },
        State {
            name: "active"
            when: control.checked || control.down

            PropertyChanges {
                target: textItem
                color: Constants.darkActiveGlobalText
            }

            PropertyChanges {
                target: buttonBackground
                color: Constants.currentBrand
                border.color: "#00000000"
            }
        }
    ]
}
