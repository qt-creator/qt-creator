// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    id: splashBackground
    width: 460
    height: 480
    color: "#00ffffff"
    visible: true

    Image {
        id: image
        x: -228
        y: -66
        scale: 0.5
        source: "welcome_windows/splash_pngs/UI_Background.png"
    }

    RectangleMask {
        id: rectangleMask
        x: 23
        y: 156
        width: 209
        height: 199
        scale: 0.5
    }

    Image1 {
        id: sliderBackground
        x: 155
        y: 267
        height: 86
    }

    SlidersTogether {
        id: slidersTogether
        x: 185
        y: 295
    }

    Image {
        id: slidersOff
        x: 191
        y: 316
        scale: 0.5
        source: "welcome_windows/splash_pngs/sliders_off.png"
    }

    Image2 {
        id: rings
        x: 84
        y: 227
        scale: 0.5
    }

    ArcAnimation {
        id: arcAnimation
        x: 9
        y: 13
        scale: 0.5
    }

    Sequencer {
        id: sequencer
        x: 152
        y: 119
        anchors.rightMargin: 119
        anchors.bottomMargin: 264
        anchors.leftMargin: 154
        anchors.topMargin: 108
    }
}
