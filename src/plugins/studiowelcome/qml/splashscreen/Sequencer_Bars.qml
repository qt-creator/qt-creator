/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.0
import QtQuick.Timeline 1.0

Item {
    id: item1
    width: 200
    height: 100

    Rectangle {
        id: mask_1
        x: 0
        y: 8
        width: 7
        height: 4
        color: "#00000000"
        clip: true

        Image {
            id: bar_1
            width: 72
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 8
            source: "welcome_windows/sequencer_images/seq_bar_1.png"
        }
    }

    Rectangle {
        id: mask_2
        x: 0
        y: 13
        width: 6
        height: 6
        color: "#00000000"
        clip: true

        Image {
            id: bar_2
            width: 55
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            anchors.top: parent.top
            anchors.topMargin: 0
            source: "welcome_windows/sequencer_images/seq_bar_2.png"
        }
    }

    Rectangle {
        id: mask_3
        x: 0
        y: 21
        width: 50
        height: 5
        color: "#00000000"
        clip: true

        Image {
            id: bar_3
            x: 8
            y: 0
            width: 36
            height: 5
            source: "welcome_windows/sequencer_images/seq_bar_3.png"
        }
    }

    Rectangle {
        id: mask_4
        x: 0
        y: 28
        width: 99
        height: 4
        color: "#00000000"
        clip: true

        Image {
            id: bar_4
            width: 79
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            anchors.top: parent.top
            anchors.topMargin: 0
            source: "welcome_windows/sequencer_images/seq_bar_4.png"
        }
    }

    Rectangle {
        id: mask_5
        x: 115
        y: 8
        width: 85
        height: 4
        color: "#00000000"
        clip: true

        Image {
            id: bar_5
            width: 72
            anchors.left: parent.left
            anchors.leftMargin: 5
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            anchors.top: parent.top
            anchors.topMargin: 0
            source: "welcome_windows/sequencer_images/seq_bar_5.png"
        }
    }

    Rectangle {
        id: mask_6
        x: 115
        y: 13
        width: 69
        height: 6
        color: "#00000000"
        clip: true

        Image {
            id: bar_6
            width: 45
            anchors.left: parent.left
            anchors.leftMargin: 5
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            anchors.top: parent.top
            anchors.topMargin: 0
            source: "welcome_windows/sequencer_images/seq_bar_6.png"
        }
    }

    Rectangle {
        id: mask_7
        x: 115
        y: 21
        width: 4
        height: 5
        color: "#00000000"
        clip: true

        Image {
            id: bar_7
            width: 64
            anchors.left: parent.left
            anchors.leftMargin: 5
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            anchors.top: parent.top
            anchors.topMargin: 0
            source: "welcome_windows/sequencer_images/seq_bar_7.png"
        }
    }

    Timeline {
        id: timeline
        enabled: true
        startFrame: 0
        endFrame: 1000

        KeyframeGroup {
            target: mask_1
            property: "width"
            Keyframe {
                frame: 0
                value: 6
            }

            Keyframe {
                easing.bezierCurve: [0.25, 0.46, 0.45, 0.94, 1, 1]
                frame: 499
                value: 87
            }

            Keyframe {
                frame: 999
                value: 6
            }
        }

        KeyframeGroup {
            target: mask_1
            property: "height"
            Keyframe {
                frame: 0
                value: 4
            }

            Keyframe {
                easing.bezierCurve: [0.25, 0.46, 0.45, 0.94, 1, 1]
                frame: 499
                value: 4
            }

            Keyframe {
                frame: 999
                value: 4
            }
        }

        KeyframeGroup {
            target: mask_2
            property: "width"
            Keyframe {
                frame: 0
                value: 6
            }

            Keyframe {
                frame: 151
                value: 7
            }

            Keyframe {
                easing.bezierCurve: [0.55, 0.08, 0.68, 0.53, 1, 1]
                frame: 677
                value: 64
            }

            Keyframe {
                frame: 999
                value: 6
            }
        }

        KeyframeGroup {
            target: mask_2
            property: "height"
            Keyframe {
                frame: 0
                value: 6
            }

            Keyframe {
                frame: 151
                value: 6
            }

            Keyframe {
                easing.bezierCurve: [0.55, 0.08, 0.68, 0.53, 1, 1]
                frame: 677
                value: 6
            }

            Keyframe {
                frame: 999
                value: 6
            }
        }

        KeyframeGroup {
            target: mask_3
            property: "width"
            Keyframe {
                frame: 0
                value: 6
            }

            Keyframe {
                easing.bezierCurve: [0.25, 0.25, 0.75, 0.75, 1, 1]
                frame: 317
                value: 47
            }

            Keyframe {
                frame: 999
                value: 6
            }

            Keyframe {
                easing.bezierCurve: [0.17, 0.84, 0.44, 1.00, 1, 1]
                frame: 544
                value: 50
            }
        }

        KeyframeGroup {
            target: mask_3
            property: "height"
            Keyframe {
                frame: 0
                value: 5
            }

            Keyframe {
                easing.bezierCurve: [0.25, 0.25, 0.75, 0.75, 1, 1]
                frame: 317
                value: 5
            }

            Keyframe {
                frame: 999
                value: 5
            }

            Keyframe {
                easing.bezierCurve: [0.17, 0.84, 0.44, 1.00, 1, 1]
                frame: 544
                value: 5
            }
        }

        KeyframeGroup {
            target: mask_4
            property: "width"
            Keyframe {
                frame: 0
                value: 6
            }

            Keyframe {
                frame: 243
                value: 6
            }

            Keyframe {
                easing.bezierCurve: [0.00, 0.00, 0.20, 1.00, 1, 1]
                frame: 682
                value: 92
            }

            Keyframe {
                frame: 755
                value: 90
            }

            Keyframe {
                frame: 999
                value: 6
            }
        }

        KeyframeGroup {
            target: mask_4
            property: "height"
            Keyframe {
                frame: 0
                value: 4
            }

            Keyframe {
                frame: 243
                value: 4
            }

            Keyframe {
                easing.bezierCurve: [0.00, 0.00, 0.20, 1.00, 1, 1]
                frame: 682
                value: 4
            }

            Keyframe {
                frame: 755
                value: 4
            }

            Keyframe {
                frame: 999
                value: 4
            }
        }

        KeyframeGroup {
            target: mask_5
            property: "width"
            Keyframe {
                frame: 0
                value: 3
            }

            Keyframe {
                frame: 182
                value: 3
            }

            Keyframe {
                easing.bezierCurve: [0.77, 0.00, 0.17, 1.00, 1, 1]
                frame: 462
                value: 85
            }

            Keyframe {
                frame: 625
                value: 85
            }

            Keyframe {
                frame: 999
                value: 4
            }
        }

        KeyframeGroup {
            target: mask_5
            property: "height"
            Keyframe {
                frame: 0
                value: 4
            }

            Keyframe {
                frame: 182
                value: 4
            }

            Keyframe {
                easing.bezierCurve: [0.77, 0.00, 0.17, 1.00, 1, 1]
                frame: 462
                value: 4
            }

            Keyframe {
                frame: 625
                value: 4
            }

            Keyframe {
                frame: 999
                value: 4
            }
        }

        KeyframeGroup {
            target: mask_6
            property: "width"
            Keyframe {
                frame: 0
                value: 3
            }

            Keyframe {
                frame: 364
                value: 3
            }

            Keyframe {
                easing.bezierCurve: [0.19, 1.00, 0.22, 1.00, 1, 1]
                frame: 695
                value: 54
            }

            Keyframe {
                frame: 999
                value: 4
            }
        }

        KeyframeGroup {
            target: mask_6
            property: "height"
            Keyframe {
                frame: 0
                value: 6
            }

            Keyframe {
                frame: 364
                value: 6
            }

            Keyframe {
                easing.bezierCurve: [0.19, 1.00, 0.22, 1.00, 1, 1]
                frame: 695
                value: 6
            }

            Keyframe {
                frame: 999
                value: 6
            }
        }

        KeyframeGroup {
            target: mask_7
            property: "width"
            Keyframe {
                frame: 0
                value: 3
            }

            Keyframe {
                frame: 444
                value: 3
            }

            Keyframe {
                easing.bezierCurve: [0.00, 0.00, 0.20, 1.00, 1, 1]
                frame: 759
                value: 77
            }

            Keyframe {
                frame: 845
                value: 77
            }

            Keyframe {
                frame: 999
                value: 4
            }
        }

        KeyframeGroup {
            target: mask_7
            property: "height"
            Keyframe {
                frame: 0
                value: 5
            }

            Keyframe {
                frame: 444
                value: 5
            }

            Keyframe {
                easing.bezierCurve: [0.00, 0.00, 0.20, 1.00, 1, 1]
                frame: 759
                value: 5
            }

            Keyframe {
                frame: 845
                value: 5
            }

            Keyframe {
                frame: 999
                value: 5
            }
        }

        KeyframeGroup {
            target: mask_8
            property: "width"
            Keyframe {
                frame: 0
                value: 3
            }

            Keyframe {
                easing.bezierCurve: [0.55, 0.06, 0.68, 0.19, 1, 1]
                frame: 303
                value: 85
            }

            Keyframe {
                frame: 516
                value: 85
            }

            Keyframe {
                frame: 999
                value: 4
            }
        }

        KeyframeGroup {
            target: mask_8
            property: "height"
            Keyframe {
                frame: 0
                value: 4
            }

            Keyframe {
                easing.bezierCurve: [0.55, 0.06, 0.68, 0.19, 1, 1]
                frame: 303
                value: 4
            }

            Keyframe {
                frame: 516
                value: 4
            }

            Keyframe {
                frame: 999
                value: 4
            }
        }
    }

    PropertyAnimation {
        id: propertyAnimation
        target: timeline
        property: "currentFrame"
        running: true
        duration: 3400
        loops: -1
        from: timeline.startFrame
        to: timeline.endFrame
    }

    Rectangle {
        id: mask_8
        x: 115
        y: 28
        width: 85
        height: 4
        color: "#00000000"
        clip: true

        Image {
            id: bar_8
            x: 5
            y: 0
            width: 72
            height: 4
            source: "welcome_windows/sequencer_images/seq_bar_8.png"
        }
    }
}
