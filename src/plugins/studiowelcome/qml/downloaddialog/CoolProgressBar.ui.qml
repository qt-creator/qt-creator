// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


import QtQuick 2.12
import QtQuick.Timeline 1.0

Item {
    id: coolProgressBar
    width: 605
    height: 16
    property alias value: timeline.currentFrame
    clip: true

    Timeline {
        id: timeline
        enabled: true
        endFrame: 100
        startFrame: 0

        KeyframeGroup {
            target: rectangle
            property: "width"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 150
                frame: 25
            }

            Keyframe {
                value: 300
                frame: 50
            }

            Keyframe {
                value: 450
                frame: 75
            }

            Keyframe {
                value: 600
                frame: 100
            }
        }

        KeyframeGroup {
            target: rectangle1
            property: "width"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 300
                frame: 25
            }

            Keyframe {
                value: 450
                frame: 50
            }

            Keyframe {
                value: 600
                frame: 75
            }
        }

        KeyframeGroup {
            target: rectangle2
            property: "width"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 450
                frame: 25
            }

            Keyframe {
                value: 600
                frame: 50
            }
        }

        KeyframeGroup {
            target: rectangle3
            property: "width"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 600
                frame: 25
            }
        }

        KeyframeGroup {
            target: content
            property: "opacity"
            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 1
                frame: 50
            }
        }
    }

    Item {
        id: content
        anchors.fill: parent

        Rectangle {
            id: rectangle
            y: 0
            width: 80
            height: 16
            color: "#ffffff"
            radius: 12
        }

        Rectangle {
            id: rectangle1
            y: 0
            width: 80
            height: 16
            opacity: 0.6
            color: "#ffffff"
            radius: 12
        }

        Rectangle {
            id: rectangle2
            y: 0
            width: 80
            height: 16
            opacity: 0.4
            color: "#ffffff"
            radius: 12
        }

        Rectangle {
            id: rectangle3
            y: 0
            width: 80
            height: 16
            opacity: 0.2
            color: "#ffffff"
            radius: 12
        }
    }
}

/*##^##
Designer {
    D{i:0;height:16;width:590}D{i:1}
}
##^##*/
