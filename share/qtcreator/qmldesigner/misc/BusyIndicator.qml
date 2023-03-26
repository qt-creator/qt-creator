// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls

BusyIndicator {
    id: control

    contentItem: Item {
        implicitWidth: 64
        implicitHeight: 64

        Item {
            id: item
            anchors.centerIn: parent
            width: 64
            height: 64
            opacity: control.running ? 1 : 0

            Behavior on opacity {
                OpacityAnimator {
                    duration: 250
                }
            }

            RotationAnimator {
                target: item
                running: control.visible && control.running
                from: 0
                to: 360
                loops: Animation.Infinite
                duration: 2000
            }

            Repeater {
                id: repeater
                model: 6

                Rectangle {
                    id: delegate
                    anchors.centerIn: parent
                    implicitWidth: 10
                    implicitHeight: 10
                    radius: 5
                    color: "#2aafd3"

                    required property int index

                    transform: [
                        Translate {
                            y: -Math.min(item.width, item.height) * 0.5 + 5
                        },
                        Rotation {
                            angle: delegate.index / repeater.count * 360
                            origin.x: 5
                            origin.y: 5
                        }
                    ]
                }
            }
        }
    }
}
