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

Column {
    id: root

    property color color
    property real alpha: 1

    property real hue: 0
    property real saturation: 0
    property real lightness: 0

    property int sliderMargins: 6

    property bool block: false

    signal updateColor
    signal rightMouseButtonClicked
    signal colorInvalidated

    spacing: 10

    onAlphaChanged: invalidateColor()
    onSaturationChanged: invalidateColor()
    onLightnessChanged: invalidateColor()
    onHueChanged: invalidateColor()
    onColorChanged: {
        var myAlpha = root.color.a
        rgbToHsl(root.color)
        root.alpha = myAlpha
    }

    function invalidateColor() {
        if (root.block)
            return

        root.block = true

        root.color = Qt.hsla(root.hue,
                             root.saturation,
                             root.lightness,
                             root.alpha)

        if (root.saturation > 0.0 && root.lightness > 0.0)
            hueSlider.value = root.hue

        opacitySlider.value = (1.0 - root.alpha)

        root.colorInvalidated()

        root.block = false
    }

    function rgbToHsl(color) {
        var r = color.r
        var g = color.g
        var b = color.b

        var max = Math.max(r, g, b), min = Math.min(r, g, b)
        var h, s, l = (max + min) / 2

        if (max === min) {
            h = 0
            s = 0
        } else {
            var d = max - min
            s = l > 0.5 ? d / (2 - max - min) : d / (max + min)
            switch (max) {
                case r: h = (g - b) / d + (g < b ? 6 : 0); break;
                case g: h = (b - r) / d + 2; break;
                case b: h = (r - g) / d + 4; break;
            }

            h /= 6
        }

        root.block = true

        if (s > 0)
            root.hue = h

        root.saturation = s
        root.lightness = l

        root.block = false
        invalidateColor()
    }

    Rectangle {
        width: parent.width
        height: width
        color: "#00000000"
        border.color: StudioTheme.Values.themeControlOutline
        border.width: StudioTheme.Values.border

        Image {
            id: checkerboard
            x: StudioTheme.Values.border
            y: StudioTheme.Values.border
            width: parent.width - 2 * StudioTheme.Values.border
            height: width

            source: "images/checkers.png"
            fillMode: Image.Tile

            // Note: We smoothscale the shader from a smaller version to improve performance
            Canvas {
                id: gradientOverlay

                property real hue: root.hue

                anchors.fill: parent
                opacity: root.alpha

                onHueChanged: requestPaint()
                onPaint: {
                    var ctx = gradientOverlay.getContext('2d')
                    ctx.save()
                    ctx.clearRect(0, 0, gradientOverlay.width, gradientOverlay.height)

                    for (var row = 0; row < gradientOverlay.height; row++) {
                        var gradient = ctx.createLinearGradient(0, 0, gradientOverlay.width,0)
                        var l = Math.abs(row - gradientOverlay.height) / gradientOverlay.height

                        gradient.addColorStop(0, Qt.hsla(gradientOverlay.hue, 0, l, 1))
                        gradient.addColorStop(1, Qt.hsla(gradientOverlay.hue, 1, l, 1))

                        ctx.fillStyle = gradient
                        ctx.fillRect(0, row, gradientOverlay.width, 1)
                    }

                    ctx.restore()
                }
            }

            Canvas {
                id: pickerCross

                property real cavnasSaturation: root.saturation
                property real canvasLightness: root.lightness
                property color strokeStyle: "lightGray"

                opacity: 0.8
                anchors.fill: parent
                antialiasing: true

                onCavnasSaturationChanged: requestPaint();
                onCanvasLightnessChanged: requestPaint();
                onPaint: {
                    var ctx = pickerCross.getContext('2d')

                    ctx.save()

                    ctx.clearRect(0, 0, pickerCross.width, pickerCross.height)

                    var yy = pickerCross.height -root.lightness * pickerCross.height
                    var xx = root.saturation * pickerCross.width

                    ctx.strokeStyle = pickerCross.strokeStyle
                    ctx.lineWidth = 1

                    ctx.beginPath()
                    ctx.moveTo(0, yy)
                    ctx.lineTo(pickerCross.width, yy)
                    ctx.stroke()

                    ctx.beginPath()
                    ctx.moveTo(xx, 0)
                    ctx.lineTo(xx, pickerCross.height)
                    ctx.stroke()

                    ctx.restore()
                }
            }

            MouseArea {
                id: mapMouseArea

                anchors.fill: parent
                preventStealing: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton

                onPositionChanged: function(mouse) {
                    if (pressed && mouse.buttons === Qt.LeftButton) {
                        var xx = Math.max(0, Math.min(mouse.x, parent.width))
                        var yy = Math.max(0, Math.min(mouse.y, parent.height))

                        root.lightness = 1.0 - yy / parent.height
                        root.saturation = xx / parent.width
                    }
                }
                onPressed: function(mouse) {
                    if (mouse.button === Qt.LeftButton)
                        positionChanged(mouse)
                }
                onReleased: function(mouse) {
                    if (mouse.button === Qt.LeftButton)
                        root.updateColor()
                }
                onClicked: function(mouse) {
                    if (mouse.button === Qt.RightButton)
                        root.rightMouseButtonClicked()
                }
            }
        }
    }

    HueSlider {
        id: hueSlider
        width: parent.width
        onValueChanged: {
            if (root.hue !== value)
                root.hue = value
        }
        onClicked: root.updateColor()
    }

    OpacitySlider {
        id: opacitySlider
        width: parent.width
        color: Qt.rgba(root.color.r, root.color.g, root.color.b, 1)
        onValueChanged: {
            if (root.alpha !== value)
                root.alpha = (1.0 - value)
        }
        onClicked: root.updateColor()
    }
}
