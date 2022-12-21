// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick 2.15
import QtQuick.Templates 2.15

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
    property alias fontpixelSize: textItem.font.pixelSize
    property bool forceHover: false
    state: "normal"

    background: buttonBackground
    Rectangle {
        id: buttonBackground
        color: "#00000000"
        implicitWidth: 100
        implicitHeight: 40
        opacity: enabled ? 1 : 0.3
        radius: 2
        border.color: "#047eff"
        anchors.fill: parent
    }

    contentItem: textItem

    Text {
        id: textItem
        text: control.text
        font.pixelSize: 18

        opacity: enabled ? 1.0 : 0.3
        color: "#ffffff"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        rightPadding: 5
        leftPadding: 5
    }

    states: [
        State {
            name: "normal"
            when: !control.down && !control.hovered && !control.forceHover

            PropertyChanges {
                target: buttonBackground
                color: "#323232"
                border.color: "#868686"
            }

            PropertyChanges {
                target: textItem
                color: "#ffffff"
            }
        },
        State {
            name: "hover"
            when: (control.hovered || control.forceHover) && !control.down
            PropertyChanges {
                target: textItem
                color: "#ffffff"
            }

            PropertyChanges {
                target: buttonBackground
                color: "#474747"
                border.color: "#adadad"
            }
        },
        State {
            name: "activeQds"
            when: control.down
            PropertyChanges {
                target: textItem
                color: "#111111"
            }

            PropertyChanges {
                target: buttonBackground
                color: "#2e769e"
                border.color: "#2e769e"
            }
        }
    ]
}
