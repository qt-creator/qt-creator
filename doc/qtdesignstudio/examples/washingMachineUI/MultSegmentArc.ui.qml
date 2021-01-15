/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Design Studio.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.12
import QtQuick.Timeline 1.0

Item {
    id: root
    width: arcSegment1.width * 2 + root.borderOffest
    height: arcSegment1.height * 2 + root.borderOffest
    clip: true
    property int borderOffest: 0 //was 5
    property alias arcSegment4Source: arcSegment4.source
    property alias arcSegment3Source: arcSegment3.source
    property alias arcSegment2Source: arcSegment2.source
    property alias arcSegment1Source: arcSegment1.source
    property alias timelineCurrentFrame: timeline.currentFrame
    property color maskColor: "#000000"

    Image {
        id: arcSegment4
        anchors.left: parent.left
        anchors.top: parent.top
        source: "assets/bigArcSegment4.png"
        transformOrigin: Item.BottomRight
        rotation: 0
        fillMode: Image.PreserveAspectFit
    }

    Rectangle {
        id: mask3
        width: arcSegment1.width + root.borderOffest
        height: arcSegment1.height + root.borderOffest
        color: root.maskColor
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
    }

    Image {
        id: arcSegment3
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        source: "assets/bigArcSegment3.png"
        rotation: 0
        transformOrigin: Item.TopRight
        fillMode: Image.PreserveAspectFit
    }

    Rectangle {
        id: mask2
        width: arcSegment1.width + root.borderOffest
        height: arcSegment1.height + root.borderOffest
        color: root.maskColor
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.rightMargin: 0
    }

    Image {
        id: arcSegment2
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        source: "assets/bigArcSegment2.png"
        rotation: 0
        transformOrigin: Item.TopLeft
        fillMode: Image.PreserveAspectFit
    }

    Rectangle {
        id: mask1
        width: arcSegment1.width + root.borderOffest
        height: arcSegment1.height + root.borderOffest
        color: root.maskColor
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 0
    }

    Image {
        id: arcSegment1
        anchors.right: parent.right
        anchors.top: parent.top
        source: "assets/bigArcSegment1.png"
        rotation: 0
        transformOrigin: Item.BottomLeft
        fillMode: Image.PreserveAspectFit
    }

    Rectangle {
        id: mask4
        width: arcSegment1.width + root.borderOffest
        height: arcSegment1.height + root.borderOffest
        opacity: 0
        color: root.maskColor
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 0
    }

    Timeline {
        id: timeline
        endFrame: 1000
        startFrame: 0
        enabled: true

        KeyframeGroup {
            target: arcSegment1
            property: "rotation"

            Keyframe {
                value: -90
                frame: 0
            }

            Keyframe {
                value: 0
                frame: 250
            }
        }

        KeyframeGroup {
            target: arcSegment2
            property: "rotation"

            Keyframe {
                value: -90
                frame: 0
            }

            Keyframe {
                value: -90
                frame: 250 //was 252
            }

            Keyframe {
                value: 0
                frame: 500
            }
        }

        KeyframeGroup {
            target: arcSegment3
            property: "rotation"

            Keyframe {
                value: -90
                frame: 0
            }

            Keyframe {
                value: -90
                frame: 500
            }

            Keyframe {
                value: 0
                frame: 750
            }
        }

        KeyframeGroup {
            target: mask4
            property: "opacity"

            Keyframe {
                value: 1
                frame: 0
            }

            Keyframe {
                value: 1
                frame: 750
            }

            Keyframe {
                value: 0
                frame: 751
            }
        }

        KeyframeGroup {
            target: arcSegment4
            property: "rotation"

            Keyframe {
                value: -90
                frame: 0
            }

            Keyframe {
                value: -90
                frame: 750
            }

            Keyframe {
                value: 0
                frame: 1000
            }
        }
    }
}

