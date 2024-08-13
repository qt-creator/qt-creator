// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T

T.Button {
    id: root
    width: 29
    height: 29

    Item {
        id: closeButton
        anchors.fill: parent
        state: "idle"

        Item {
            id: closeIcon
            width: 15
            height: 15
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            rotation: 45

            Rectangle {
                id: rectangle
                width: 1
                height: 15
                color: "#ffffff"
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Rectangle {
                id: rectangle1
                width: 1
                height: 15
                color: "#ffffff"
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                rotation: 90
            }
        }
    }

    states: [
        State {
            name: "idle"
            when: !root.hovered && !root.pressed

            PropertyChanges {
                target: rectangle
                color: "#b2b2b2"
            }

            PropertyChanges {
                target: rectangle1
                color: "#b2b2b2"
            }
        },
        State {
            name: "hoverPress"
            when: (root.hovered || root.pressed)
        }
    ]
}
