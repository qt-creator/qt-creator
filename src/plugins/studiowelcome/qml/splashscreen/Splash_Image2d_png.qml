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
