/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.8
import QtQuick.Timeline 1.0
import QtQuick.Controls 2.3

Item {
    id: root
    width: 350
    height: 1080
    property alias slidermenuSlider_4BValue: slidermenu.slider_4BValue
    property alias slidermenuSlider_4AValue: slidermenu.slider_4AValue
    property alias slidermenuSlider_3CValue: slidermenu.slider_3CValue
    property alias slidermenuSlider_3BValue: slidermenu.slider_3BValue
    property alias slidermenuSlider_3AValue: slidermenu.slider_3AValue
    property alias slidermenuSlider_1BValue: slidermenu.slider_1BValue
    property alias slidermenuSlider_1CValue: slidermenu.slider_1CValue
    property alias slidermenuSlider_2BValue: slidermenu.slider_2BValue
    property alias slidermenuSlider_2AValue: slidermenu.slider_2AValue
    property alias slidermenuSlider_1AValue: slidermenu.slider_1AValue

    state: "closed"

    Image {
        id: sideMenuAsset
        x: 0
        y: 0
        visible: false
        source: "assets/rectangle_3_195_205.png"
    }

    Image {
        id: imageOutline
        x: -423
        y: 0
        height: 1079
        source: "assets/empty_rect.png"
    }

    Image {
        id: imageBackground
        x: 0
        y: 0
        source: "assets/rectangle_3_195_205.png"

        SliderMenu {
            id: slidermenu
            width: 349
            height: 1079
            slider_1AValue: -0.3
        }
    }

    BurgerMenu {
        id: burger_menu
        x: 22
        y: 20
    }

    Timeline {
        id: timeline
        enabled: true
        animations: [
            TimelineAnimation {
                id: timelineAnimation
                property: "currentFrame"
                pingPong: false
                duration: 300
                from: 0
                loops: 1
                to: 1000
                //alwaysRunToEnd: true
                running: false
                onFinished: root.state = "open"
            },
            TimelineAnimation {
                id: timelineAnimation2
                property: "currentFrame"
                pingPong: false
                duration: 700
                from: 1000
                loops: 1
                to: 0
                //alwaysRunToEnd: true
                running: false
                onFinished: root.state = "closed"
            }
        ]
        endFrame: 1000
        startFrame: 0

        KeyframeGroup {
            target: imageOutline
            property: "x"
            Keyframe {
                easing.bezierCurve: [0.337, 0.229, 0.758, 0.282, 1, 1]
                frame: 1000
                value: 0
            }

            Keyframe {
                frame: 1
                value: -348
            }
        }

        KeyframeGroup {
            target: imageBackground
            property: "x"
            Keyframe {
                frame: 402
                value: -423
            }

            Keyframe {
                frame: 0
                value: -424
            }

            Keyframe {
                easing.bezierCurve: [0.337, 0.229, 0.758, 0.282, 1, 1]
                frame: 1000
                value: 0
            }
        }
    }

    Connections {
        target: burger_menu
        onClicked: {
            root.state = "opening"
        }
        enabled: root.state === "closed"
    }

    Connections {
        target: burger_menu
        onClicked: {
            root.state = "closing"
        }
        enabled: root.state === "open"
    }

    states: [
        State {
            name: "opening"

            PropertyChanges {
                target: timelineAnimation
                running: true
            }

            PropertyChanges {
                target: burger_menu
                enabled: false
            }

            PropertyChanges {
                target: slidermenu
                visible: false
            }
        },
        State {
            name: "open"
        },
        State {
            name: "closing"
            PropertyChanges {
                target: timelineAnimation2
                running: true
            }

            PropertyChanges {
                target: burger_menu
                enabled: false
            }
        },
        State {
            name: "closed"
            PropertyChanges {
                target: slidermenu
                visible: false
            }
        }
    ]
}
