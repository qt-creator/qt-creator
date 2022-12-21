// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQuick.Timeline 1.0

Item {
    id: arcItem
    width: 549
    height: 457
    Arc {
        id: arc
        x: 401
        y: 191
        width: 52
        height: 52
        strokeColor: "#5d9dea"
        end: 0
        scale: 0.8
        strokeWidth: 7
    }

    Arc {
        id: arc1
        x: 353
        y: 203
        width: 52
        height: 52
        rotation: -45
        strokeWidth: 7
        scale: 0.6
        strokeColor: "#ea6d60"
        end: 0
    }

    Arc {
        id: arc2
        x: 390
        y: 226
        width: 52
        height: 52
        rotation: 90
        scale: 0.5
        strokeWidth: 7
        strokeColor: "#aa97f4"
        end: 0
    }

    Timeline {
        id: timeline
        startFrame: 0
        enabled: true
        endFrame: 1000

        KeyframeGroup {
            target: arc
            property: "end"

            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 280
                frame: 520
            }

            Keyframe {
                value: 0
                frame: 1000
            }
        }

        KeyframeGroup {
            target: arc1
            property: "end"

            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 320
                frame: 690
            }

            Keyframe {
                value: 0
                frame: 1000
            }
        }

        KeyframeGroup {
            target: arc2
            property: "end"

            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 290
                frame: 320
            }

            Keyframe {
                value: 0
                frame: 1000
            }
        }
    }

    PropertyAnimation {
        id: propertyAnimation
        target: timeline
        property: "currentFrame"
        running: true
        loops: -1
        duration: 2600
        from: timeline.startFrame
        to: timeline.endFrame
    }
}
