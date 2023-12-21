// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Timeline

Rectangle {
    id: root
    width: 872
    height: 860
    radius: 4
    color: "transparent"
    border.color: "#1381e3"
    border.width: 8
    state: "off"

    property bool active: true

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



