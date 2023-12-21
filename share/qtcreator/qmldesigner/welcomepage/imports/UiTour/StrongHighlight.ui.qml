// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Timeline

Rectangle {
    id: root
    width: 600
    height: 600
    radius: 2
    color: "transparent"
    border.color: "#1381e3"
    border.width: 8
    state: "off"

    property bool active: true

    property color shadowColor: "#aa1b1b1b"

    property int globalX: root.x //200
    property int globalY: root.y //200
    property int parentWidth: root.parent.width //1200
    property int parentHeight: root.parent.height //1200

    Rectangle {
        z: -1
        color: "transparent"
        anchors.fill: parent
        border.color: root.shadowColor
        border.width: 3
    }

    Rectangle {
        x: -width
        z: -1
        width: root.globalX
        height: root.height
        color: root.shadowColor
    }

    Rectangle {
        x: root.width
        z: -1
        width: root.parentWidth - root.globalX - root.width
        height: root.height
        color: root.shadowColor
    }

    Rectangle {
        x: -root.globalX
        y: -root.globalY
        z: -1
        width: root.parentWidth
        height: root.globalY
        color: root.shadowColor
    }

    Rectangle {
        x: -root.globalX
        y: root.height
        z: -1
        width: root.parentWidth
        height: root.parentHeight - root.globalY - root.height
        color: root.shadowColor
    }

    states: [
        State {
            name: "on"
            when: root.active

            PropertyChanges {
                target: root
            }
        },
        State {
            name: "off"
            when: !root.active

            PropertyChanges {
                target: root
                opacity: 0
            }
        }
    ]

    transitions: [
        Transition {
            id: transition
            to: "*"
            from: "*"
            ParallelAnimation {
                SequentialAnimation {
                    PauseAnimation { duration: 0 }

                    PropertyAnimation {
                        target: root
                        property: "opacity"
                        duration: 150
                    }
                }
            }
        }
    ]
}
