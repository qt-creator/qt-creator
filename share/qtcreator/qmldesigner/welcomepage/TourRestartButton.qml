// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import WelcomeScreen 1.0

Rectangle {
    id: restart
    height: 36
    color: "#00ffffff"
    radius: 18
    border.color: "#f9f9f9"
    border.width: 3
    state: "normal"

    signal restart()

    Text {
        id: text2
        color: "#ffffff"
        text: qsTrId("Restart")
        anchors.verticalCenter: parent.verticalCenter
        font.pixelSize: 12
        anchors.horizontalCenter: parent.horizontalCenter
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true

        Connections {
            target: mouseArea
            onClicked: restart.restart()
        }
    }

    states: [
        State {
            name: "normal"
            when: !mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: text2
                color: "#dedede"
            }

            PropertyChanges {
                target: restart
                border.color: "#dedede"
            }
        },
        State {
            name: "hover"
            when: mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: restart
                color: "#00ffffff"
                border.color: "#ffffff"
            }

            PropertyChanges {
                target: text2
                color: "#ffffff"
            }
        },
        State {
            name: "press"
            when: mouseArea.pressed

            PropertyChanges {
                target: restart
                color: "#ffffff"
                border.color: "#ffffff"
            }

            PropertyChanges {
                target: text2
                color: "#000000"
            }
        }
    ]
}
