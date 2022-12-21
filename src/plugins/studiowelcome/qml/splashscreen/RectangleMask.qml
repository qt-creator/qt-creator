// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQuick.Timeline 1.0

Item {
    width: 174
    height: 200

    Item {
        id: rectangleMask
        width: 0
        height: 200
        clip: true

        Image {
            id: image1
            x: 0
            y: 45
            source: "welcome_windows/splash_pngs/Audio_Waves.png"
        }

        Timeline {
            id: timeline
            startFrame: 0
            enabled: true
            endFrame: 2000

            KeyframeGroup {
                target: rectangleMask
                property: "width"

                Keyframe {
                    value: 0
                    frame: 0
                }

                Keyframe {
                    easing.bezierCurve: [0.25, 0.10, 0.25, 1.00, 1, 1]
                    value: 175
                    frame: 997
                }

                Keyframe {
                    easing.bezierCurve: [0.00, 0.00, 0.58, 1.00, 1, 1]
                    value: 0
                    frame: 1998
                }
            }
        }

        PropertyAnimation {
            id: propertyAnimation
            target: timeline
            property: "currentFrame"
            running: true
            loops: -1
            duration: 2000
            from: timeline.startFrame
            to: timeline.endFrame
        }
    }
}
