/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

import QtQuick 2.15
import StudioTheme 1.0 as StudioTheme

Item {
    id: root

    property real value: 1
    property real minimum: 0
    property real maximum: 1
    property bool pressed: mouseArea.pressed
    property bool integer: false
    property color color

    signal clicked

    height: StudioTheme.Values.hueSliderHeight

    function updatePos() {
        if (root.maximum > root.minimum) {
            var pos = (track.width - handle.width) * (root.value - root.minimum) / (root.maximum - root.minimum)
            return Math.min(Math.max(pos, 0), track.width - handle.width)
        } else {
            return 0
        }
    }

    Item {
        id: track

        width: parent.width
        height: parent.height

        Image {
            id: checkerboard
            anchors.fill: parent
            source: "images/checkers.png"
            fillMode: Image.Tile
        }

        Rectangle {
            anchors.fill: parent
            border.color: StudioTheme.Values.themeControlOutline
            border.width: StudioTheme.Values.border

            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.000; color: root.color }
                GradientStop { position: 1.000; color: "transparent" }
            }
        }

        Rectangle {
            id: handle
            width: StudioTheme.Values.hueSliderHandleWidth
            height: track.height - 4
            anchors.verticalCenter: parent.verticalCenter
            smooth: true
            color: "transparent"
            radius: 2
            border.color: "black"
            border.width: 1
            x: root.updatePos()
            y: 2
            z: 1

            Rectangle {
                anchors.fill: parent
                anchors.margins: 1
                color: "transparent"
                radius: 1
                border.color: "white"
                border.width: 1
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            preventStealing: true

            function calculateValue() {
                var handleX = Math.max(0, Math.min(mouseArea.mouseX, mouseArea.width))
                var realValue = (root.maximum - root.minimum) * handleX / mouseArea.width + root.minimum
                root.value = root.integer ? Math.round(realValue) : realValue
            }

            onPressed: calculateValue()
            onReleased: root.clicked()
            onPositionChanged: {
                if (pressed)
                    calculateValue()
            }
        }

    }
}
