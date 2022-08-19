// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.15
import QtQuick.Controls 2.12

Button {
    id: control
    width: 500
    height: 100

    implicitWidth: Math.max(
                       buttonBackground ? buttonBackground.implicitWidth : 0,
                       textItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(
                        buttonBackground ? buttonBackground.implicitHeight : 0,
                        textItem.implicitHeight + topPadding + bottomPadding)
    leftPadding: 4
    rightPadding: 4

    text: "My Button"
    hoverEnabled: false

    background: buttonBackground
    Rectangle {
        id: buttonBackground
        color: "#00000000"
        implicitWidth: 100
        implicitHeight: 40
        opacity: enabled ? 1 : 0.3
        radius: 50
        border.color: "#ffffff"
    }

    contentItem: textItem
    Text {
        id: textItem
        text: control.text
        font.pixelSize: 34

        opacity: enabled ? 1.0 : 0.3
        color: "#ffffff"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        leftPadding: 50
        font.family: "Titillium Web ExtraLight"
    }

    states: [
        State {
            name: "normal"
            when: !control.down

            PropertyChanges {
                target: buttonBackground
                color: "#00000000"
                border.color: "#ffffff"
            }

            PropertyChanges {
                target: textItem
                color: "#ffffff"
                font.pixelSize: 34
                font.family: "Titillium Web ExtraLight"
            }
        },
        State {
            name: "down"
            when: control.down
            PropertyChanges {
                target: textItem
                color: "#ffffff"
                border.color: "#ffffff"
                font.family: "Titillium Web ExtraLight"
            }

            PropertyChanges {
                target: buttonBackground
                color: "#28e7e7e7"
                border.color: "#00000000"
            }
        }
    ]
}
