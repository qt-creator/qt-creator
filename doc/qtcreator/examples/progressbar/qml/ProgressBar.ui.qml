

// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick 2.9
import QtQuick.Window 2.3
import QtQuick.Timeline 1.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.3

Rectangle {
    visible: true
    width: 640
    height: 480
    color: "#242424"

    ColumnLayout {
        x: 20
        y: 152
        spacing: 20

        Root {
            id: root
        }

        Root {
            id: root1
        }

        Root {
            id: root2
        }
    }

    Timeline {
        id: timeline
        enabled: true
        endFrame: 4000
        startFrame: 0

        KeyframeGroup {
            target: root
            property: "progress"

            Keyframe {
                value: 10
                frame: 0
            }

            Keyframe {
                easing.bezierCurve: [0.86, 0.00, 0.07, 1.00, 1, 1]
                value: 90
                frame: 2000
            }

            Keyframe {
                easing.bezierCurve: [0.86, 0.00, 0.07, 1.00, 1, 1]
                value: 10
                frame: 4000
            }
        }

        KeyframeGroup {
            target: root1
            property: "progress"

            Keyframe {
                easing.bezierCurve: [0.17, 0.84, 0.44, 1.00, 1, 1]
                value: 90
                frame: 4000
            }

            Keyframe {
                easing.bezierCurve: [0.17, 0.84, 0.44, 1.00, 1, 1]
                value: 20
                frame: 2000
            }

            Keyframe {
                value: 90
                frame: 0
            }
        }

        KeyframeGroup {
            target: root2
            property: "progress"

            Keyframe {
                value: 15
                frame: 0
            }

            Keyframe {
                easing.bezierCurve: [0.79, 0.14, 0.15, 0.86, 1, 1]
                value: 85
                frame: 2000
            }

            Keyframe {
                easing.bezierCurve: [0.79, 0.14, 0.15, 0.86, 1, 1]
                value: 15
                frame: 4000
            }
        }

        animations: [
            TimelineAnimation {
                id: propertyAnimation
                target: timeline
                property: "currentFrame"
                running: true
                to: timeline.endFrame
                from: timeline.startFrame
                loops: -1
                duration: 1000
            }
        ]
    }

    Image {
        id: image
        x: 518
        y: 0
        width: 102
        height: 137
        fillMode: Image.PreserveAspectFit
        source: "built-with-Qt_Large.png"
    }
}
