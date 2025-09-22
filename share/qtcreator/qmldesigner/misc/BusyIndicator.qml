// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls

BusyIndicator {
    id: control

    property int radius: 64
    property int dotSize: 10
    property color color: "#2aafd3"

    contentItem: Item {
        implicitWidth: control.radius
        implicitHeight: control.radius

        Item {
            id: item
            anchors.centerIn: parent
            width: control.radius
            height: control.radius
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
                    implicitWidth: control.dotSize
                    implicitHeight: control.dotSize
                    radius: control.dotSize * .5
                    color: control.color

                    required property int index

                    transform: [
                        Translate {
                            y: -Math.min(item.width, item.height) * 0.5 + delegate.radius
                        },
                        Rotation {
                            angle: delegate.index / repeater.count * 360
                            origin.x: delegate.radius
                            origin.y: delegate.radius
                        }
                    ]
                }
            }
        }
    }
}
