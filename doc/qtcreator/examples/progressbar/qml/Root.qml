/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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
