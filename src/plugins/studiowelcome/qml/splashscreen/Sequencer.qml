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
    id: blocks
    anchors.right: parent.right
    anchors.rightMargin: 175
    anchors.left: parent.left
    anchors.leftMargin: 0
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 77
    anchors.top: parent.top
    anchors.topMargin: 0

    Image {
        id: sequencer
        width: 175
        height: 77
        clip: false
        fillMode: Image.PreserveAspectFit
        source: "welcome_windows/splash_pngs/Step_sequencer.png"

        Image {
            id: blue_seq_block2
            x: 3
            y: 16
            width: 10
            height: 5
            source: "welcome_windows/sequencer_images/blue_seq.png"
            fillMode: Image.PreserveAspectFit
        }

        Image {
            id: peach_seq_block
            x: 49
            y: 24
            width: 10
            height: 5
            fillMode: Image.PreserveAspectFit
            source: "welcome_windows/sequencer_images/peach_seq.png"
        }

        Image {
            id: pink_seq_block
            x: 26
            y: 23
            width: 10
            height: 5
            fillMode: Image.PreserveAspectFit
            source: "welcome_windows/sequencer_images/pink_seq.png"
        }

        Image {
            id: blue_seq_block
            x: 83
            y: 23
            width: 10
            height: 5
            fillMode: Image.PreserveAspectFit
            source: "welcome_windows/sequencer_images/blue_seq.png"
        }

        Image {
            id: pink_seq_block1
            x: 26
            y: 16
            width: 10
            height: 5
            source: "welcome_windows/sequencer_images/pink_seq.png"
            fillMode: Image.PreserveAspectFit
        }

        Image {
            id: peach_seq_block1
            x: 15
            y: 31
            width: 10
            height: 5
            source: "welcome_windows/sequencer_images/peach_seq.png"
            fillMode: Image.PreserveAspectFit
        }

        Image {
            id: pink_seq_block2
            x: 49
            y: 16
            width: 10
            height: 5
            source: "welcome_windows/sequencer_images/pink_seq.png"
            fillMode: Image.PreserveAspectFit
        }

        Image {
            id: blue_seq_block3
            x: 61
            y: 29
            width: 10
            height: 5
            source: "welcome_windows/sequencer_images/blue_seq.png"
            fillMode: Image.PreserveAspectFit
        }

        Image {
            id: pink_seq_block3
            x: 106
            y: 23
            width: 10
            height: 5
            source: "welcome_windows/sequencer_images/pink_seq.png"
            fillMode: Image.PreserveAspectFit
        }

        Image {
            id: pink_seq_block4
            x: 129
            y: 16
            width: 10
            height: 5
            source: "welcome_windows/sequencer_images/pink_seq.png"
            fillMode: Image.PreserveAspectFit
        }

        Image {
            id: blue_seq_block4
            x: 106
            y: 16
            width: 10
            height: 5
            source: "welcome_windows/sequencer_images/blue_seq.png"
            fillMode: Image.PreserveAspectFit
        }

        Image {
            id: peach_seq_block2
            x: 118
            y: 30
            width: 10
            height: 5
            source: "welcome_windows/sequencer_images/peach_seq.png"
            fillMode: Image.PreserveAspectFit
        }

        Image {
            id: peach_seq_block3
            x: 141
            y: 30
            width: 10
            height: 5
            source: "welcome_windows/sequencer_images/peach_seq.png"
            fillMode: Image.PreserveAspectFit
        }
    }

    Image {
        id: blue_seq_block1
        x: 3
        y: 23
        width: 10
        height: 5
        source: "welcome_windows/sequencer_images/blue_seq.png"
        fillMode: Image.PreserveAspectFit
    }

    Sequencer_Bars {
        id: seq_bars
        x: -7
        y: 39
        width: 199
        height: 69
    }

    Timeline {
        id: timeline
        enabled: true
        startFrame: 0
        endFrame: 1000

        KeyframeGroup {
            target: blue_seq_block2
            property: "opacity"

            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 52
                value: 0
            }

            Keyframe {
                frame: 53
                value: 1
            }

            Keyframe {
                frame: 300
                value: 1
            }

            Keyframe {
                frame: 301
                value: 0
            }
        }

        KeyframeGroup {
            target: blue_seq_block1
            property: "opacity"

            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 100
                value: 0
            }

            Keyframe {
                frame: 101
                value: 1
            }

            Keyframe {
                frame: 340
                value: 1
            }

            Keyframe {
                frame: 341
                value: 0
            }
        }

        KeyframeGroup {
            target: peach_seq_block1
            property: "opacity"

            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 175
                value: 0
            }

            Keyframe {
                frame: 176
                value: 1
            }

            Keyframe {
                frame: 432
                value: 1
            }

            Keyframe {
                frame: 435
                value: 0
            }
        }

        KeyframeGroup {
            target: pink_seq_block1
            property: "opacity"

            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 222
                value: 0
            }

            Keyframe {
                frame: 227
                value: 1
            }

            Keyframe {
                frame: 508
                value: 1
            }

            Keyframe {
                frame: 512
                value: 0
            }
        }

        KeyframeGroup {
            target: pink_seq_block
            property: "opacity"

            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 247
                value: 0
            }

            Keyframe {
                frame: 251
                value: 1
            }

            Keyframe {
                frame: 524
                value: 1
            }

            Keyframe {
                frame: 529
                value: 0
            }
        }

        KeyframeGroup {
            target: pink_seq_block2
            property: "opacity"

            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 286
                value: 0
            }

            Keyframe {
                frame: 290
                value: 1
            }

            Keyframe {
                frame: 575
                value: 1
            }

            Keyframe {
                frame: 580
                value: 0
            }
        }

        KeyframeGroup {
            target: peach_seq_block
            property: "opacity"

            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 352
                value: 0
            }

            Keyframe {
                frame: 357
                value: 1
            }

            Keyframe {
                frame: 647
                value: 1
            }

            Keyframe {
                frame: 652
                value: 0
            }
        }

        KeyframeGroup {
            target: blue_seq_block3
            property: "opacity"

            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 419
                value: 0
            }

            Keyframe {
                frame: 424
                value: 1
            }

            Keyframe {
                frame: 695
                value: 1
            }

            Keyframe {
                frame: 699
                value: 0
            }
        }

        KeyframeGroup {
            target: blue_seq_block
            property: "opacity"

            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 461
                value: 0
            }

            Keyframe {
                frame: 756
                value: 1
            }

            Keyframe {
                frame: 761
                value: 0
            }
        }

        KeyframeGroup {
            target: blue_seq_block4
            property: "opacity"

            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 502
                value: 0
            }

            Keyframe {
                frame: 508
                value: 1
            }

            Keyframe {
                frame: 799
                value: 1
            }

            Keyframe {
                frame: 804
                value: 0
            }
        }

        KeyframeGroup {
            target: pink_seq_block3
            property: "opacity"

            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 525
                value: 0
            }

            Keyframe {
                frame: 530
                value: 1
            }

            Keyframe {
                frame: 840
                value: 1
            }

            Keyframe {
                frame: 848
                value: 0
            }
        }

        KeyframeGroup {
            target: peach_seq_block2
            property: "opacity"

            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 661
                value: 0
            }

            Keyframe {
                frame: 667
                value: 1
            }

            Keyframe {
                frame: 901
                value: 1
            }

            Keyframe {
                frame: 906
                value: 0
            }
        }

        KeyframeGroup {
            target: pink_seq_block4
            property: "opacity"

            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 756
                value: 0
            }

            Keyframe {
                frame: 760
                value: 1
            }

            Keyframe {
                frame: 955
                value: 1
            }

            Keyframe {
                frame: 961
                value: 0
            }
        }

        KeyframeGroup {
            target: peach_seq_block3
            property: "opacity"

            Keyframe {
                frame: 0
                value: 0
            }

            Keyframe {
                frame: 831
                value: 0
            }

            Keyframe {
                frame: 837
                value: 1
            }

            Keyframe {
                frame: 989
                value: 1
            }

            Keyframe {
                frame: 999
                value: 0
            }
        }
    }

    PropertyAnimation {
        id: propertyAnimation
        target: timeline
        property: "currentFrame"
        running: true
        duration: 1200
        loops: -1
        from: timeline.startFrame
        to: timeline.endFrame
    }
}
