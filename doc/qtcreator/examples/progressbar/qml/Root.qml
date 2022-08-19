// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.9
import QtQuick.Timeline 1.0

Item {
    id: root
    width: 600
    height: 65
    property alias progress: timeline.currentFrame

    Rectangle {
        id: pb_back
        color: "#9f9f9f"
        radius: 4
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 30

        Rectangle {
            id: pb_front
            width: 200
            color: "#ffffff"
            anchors.bottomMargin: 5
            anchors.left: parent.left
            anchors.leftMargin: 5
            anchors.bottom: parent.bottom
            anchors.top: parent.top
            anchors.topMargin: 5
        }
    }

    Text {
        id: text1
        color: "#ffffff"
        text: Math.round(root.progress)
        font.pixelSize: 18
    }

    Timeline {
        id: timeline
        enabled: true
        startFrame: 0
        endFrame: 100

        KeyframeGroup {
            target: text1
            property: "color"

            Keyframe {
                value: "#8de98d"
                frame: 0
            }

            Keyframe {
                value: "#de4f4f"
                frame: 50
            }

            Keyframe {
                value: "#f0c861"
                frame: 100
            }
        }

        KeyframeGroup {
            target: pb_front
            property: "width"

            Keyframe {
                value: 0
                frame: 0
            }

            Keyframe {
                value: 590
                frame: 100
            }
        }

        KeyframeGroup {
            target: pb_front
            property: "color"
            Keyframe {
                value: "#8de98d"
                frame: 0
            }

            Keyframe {
                value: "#de4f4f"
                frame: 50
            }

            Keyframe {
                value: "#f0c861"
                frame: 100
            }
        }
    }
}
