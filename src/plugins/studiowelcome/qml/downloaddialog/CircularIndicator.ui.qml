// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


import QtQuick 2.12
import QtQuick.Timeline 1.0

Rectangle {
    id: rectangle
    width: 60
    height: 60
    color: "#8c8c8c"
    radius: 50
    property alias inputMax: rangeMapper.inputMax
    property alias inputMin: rangeMapper.inputMin
    property alias value: minMaxMapper.input

    ArcItem {
        id: arc
        x: -1
        y: -1
        width: 62
        height: 62
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        end: rangeMapper.value
        antialiasing: true
        strokeWidth: 8
        strokeColor: "#ffffff"
        capStyle: 32
        fillColor: "#00000000"
    }

    RangeMapper {
        id: rangeMapper
        outputMax: 358
        input: minMaxMapper.value
    }

    MinMaxMapper {
        id: minMaxMapper
        input: 95
        max: rangeMapper.inputMax
        min: rangeMapper.inputMin
    }

    Rectangle {
        id: rectangle1
        width: 60
        height: 60
        color: "#ffffff"
        radius: 40
        anchors.verticalCenter: parent.verticalCenter
        scale: 1
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Timeline {
        id: timeline
        currentFrame: rangeMapper.value
        enabled: true
        endFrame: 360
        startFrame: 0

        KeyframeGroup {
            target: rectangle1
            property: "opacity"
            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 360
                value: 1
            }
        }

        KeyframeGroup {
            target: rectangle1
            property: "scale"
            Keyframe {
                frame: 360
                value: 1
            }

            Keyframe {
                frame: 0
                value: 0.1
            }
        }

        KeyframeGroup {
            target: arc
            property: "opacity"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 1
                frame: 40
            }
        }
    }
}
