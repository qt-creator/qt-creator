// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    signal moved

    height: StudioTheme.Values.hueSliderHeight

    function updatePos() {
        if (root.maximum > root.minimum) {
            var pos = track.width * (root.value - root.minimum) / (root.maximum - root.minimum)
            return Math.min(Math.max(pos, 0), track.width)
        } else {
            return 0
        }
    }

    Item {
        id: track
        width: parent.width
        height: parent.height

        Rectangle {
            anchors.fill: parent
            border.color: StudioTheme.Values.themeControlOutline
            border.width: StudioTheme.Values.border

            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.000; color: Qt.hsva(root.color.hsvHue, root.color.hsvSaturation, 1, 1) }
                GradientStop { position: 1.000; color: "black" }
            }
        }

        Rectangle {
            id: handle
            width: StudioTheme.Values.hueSliderHandleWidth
            height: track.height + 4
            anchors.verticalCenter: parent.verticalCenter
            color: "transparent"
            radius: 2
            border.color: "black"
            border.width: 1
            x: root.updatePos() - handle.width * 0.5
            y: 2
            z: 1

            Rectangle {
                anchors.fill: parent
                anchors.margins: 1
                color: Qt.hsva(root.color.hsvHue, root.color.hsvSaturation, root.color.hsvValue, 1)
                radius: 1
                border.color: "white"
                border.width: 1
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            anchors.margins: -StudioTheme.Values.hueSliderHandleWidth * 0.5
            preventStealing: true

            function calculateValue() {
                var halfHandle = StudioTheme.Values.hueSliderHandleWidth * 0.5
                var handleX = Math.max(0, Math.min(mouseArea.mouseX - halfHandle, parent.width))
                var realValue = (root.maximum - root.minimum) * handleX / parent.width + root.minimum
                root.value = root.integer ? Math.round(realValue) : realValue
                root.moved()
            }

            onPressed: mouseArea.calculateValue()
            onReleased: root.clicked()
            onPositionChanged: {
                if (mouseArea.pressed)
                    mouseArea.calculateValue()
            }
        }

    }
}
