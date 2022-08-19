// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
