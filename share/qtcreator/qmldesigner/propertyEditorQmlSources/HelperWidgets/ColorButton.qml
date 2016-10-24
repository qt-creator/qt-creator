/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.1
import QtQuick.Controls 1.0 as Controls

Item {
    id: colorButton
    width: 64
    height: 64

    property color color
    property real alpha: 1

    property real hue: 0
    property real saturation: 0
    property real lightness: 0

    property int sliderMargins: 6

    onHueChanged: {
        //hueSlider.value = hue;
        invalidateColor();
    }

    signal clicked

    onAlphaChanged: invalidateColor();

    onSaturationChanged: invalidateColor();

    onLightnessChanged: invalidateColor();

    onColorChanged: {
        var myAlpha = color.a
        rgbToHsl(color);

        colorButton.alpha = myAlpha
    }

    property bool block: false

    function invalidateColor() {
        if (block)
            return;

        block = true

        colorButton.color = Qt.hsla(hue, saturation, lightness, alpha);

        if (saturation > 0.0 && lightness > 0.0) {
            hueSlider.value = hue
            hueSlider2.value = hue
        }

        if (lightness > 0.0)
            saturationSlider.value = saturation
        else
            saturation = saturationSlider.value

        lightnessSlider.value = lightness
        alphaSlider.value = alpha

        block = false
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
            var d = max - min;
            s = l > 0.5 ? d / (2 - max - min) : d / (max + min)
            switch (max) {
                case r: h = (g - b) / d + (g < b ? 6 : 0); break;
                case g: h = (b - r) / d + 2; break;
                case b: h = (r - g) / d + 4; break;
            }

            h /= 6
        }

        block = true

        if (s > 0)
            colorButton.hue = h

        colorButton.saturation = s
        colorButton.lightness = l

        block = false
        invalidateColor()
    }

    Image {
        id: surround
        x: 2
        y: 2
        width: height
        height: parent.height - 4

        source: "images/checkers.png"
        fillMode: Image.Tile

        // note we smoothscale the shader from a smaller version to improve performance
        Canvas {
            id: hubeBox
            opacity: colorButton.alpha
            anchors.fill: parent
            property real hue: colorButton.hue
            onHueChanged: requestPaint()

            onPaint: {
                var ctx = hubeBox.getContext('2d')

                ctx.save()

                ctx.clearRect(0, 0, hubeBox.width, hubeBox.height);

                for (var row = 0; row < hubeBox.height; row++){
                    var gradient = ctx.createLinearGradient(0, 0, hubeBox.width,0);
                    var l = Math.abs(row  - hubeBox.height) / hubeBox.height

                    gradient.addColorStop(0, Qt.hsla(hubeBox.hue, 0, l, 1));
                    gradient.addColorStop(1, Qt.hsla(hubeBox.hue, 1, l, 1));

                    ctx.fillStyle = gradient;
                    ctx.fillRect(0, row, hubeBox.width, 1);
                }

                ctx.restore()

            }

        }

        Canvas {
            id: canvas

            opacity: 0.8

            anchors.fill: parent

            antialiasing: true

            property real cavnasSaturation: colorButton.saturation
            property real canvasLightness: colorButton.lightness

            onCavnasSaturationChanged: requestPaint();
            onCanvasLightnessChanged: requestPaint();

            property color strokeStyle: "lightGray"

            onPaint: {
                var ctx = canvas.getContext('2d')

                ctx.save()

                context.clearRect(0, 0, canvas.width, canvas.height);

                var yy = canvas.height -colorButton.lightness * canvas.height
                var xx = colorButton.saturation * canvas.width

                ctx.strokeStyle = canvas.strokeStyle
                ctx.lineWidth = 1


                ctx.beginPath()
                ctx.moveTo(0, yy)
                ctx.lineTo(canvas.width, yy)
                ctx.stroke()

                ctx.beginPath()
                ctx.moveTo(xx, 0)
                ctx.lineTo(xx, canvas.height)
                ctx.stroke()

                ctx.restore()
            }

        }

        MouseArea {
            id: mapMouseArea
            anchors.fill: parent
            onPositionChanged: {
                if (pressed) {
                    var xx = Math.max(0, Math.min(mouse.x, parent.width))
                    var yy = Math.max(0, Math.min(mouse.y, parent.height))

                    colorButton.lightness =  1.0 - yy / parent.height;
                    colorButton.saturation =  xx / parent.width;
                }
            }
            onPressed: positionChanged(mouse)

            onReleased: colorButton.clicked()
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            color: "#00000000"
            border.color: "black"
            border.width: 1
        }
    }


    HueSlider {
        id: hueSlider
        anchors.left: surround.right
        anchors.margins: 6
        y: 4
        height: parent.height - 8
        //value: colorButton.hue
        onValueChanged: {
            if (colorButton.hue !== value)
                colorButton.hue = value
        }
        onClicked: colorButton.clicked()

    }
    Column {

        anchors.left: hueSlider.right
        anchors.margins: colorButton.sliderMargins
        spacing: 10

        Row {
            z: 3
            spacing: 4
            Label {
                text: "H:"
                width: 16
                color: "#eee"
                elide: Text.ElideRight
                anchors.verticalCenter: parent.verticalCenter
            }
            DoubleSpinBox {
                id: hueSlider2
                //value: colorButton.hue
                onValueChanged: {
                    if (colorButton.hue !== value  && !colorButton.block) {
                        colorButton.hue = value
                        colorButton.clicked()
                    }
                }
            }
        }

        Row {
            z: 2
            spacing: 4
            Controls.Label {
                text: "S:"
                width: 16
                color: "#eee"
                elide: Text.ElideRight
                anchors.verticalCenter: parent.verticalCenter
            }

            DoubleSpinBox {
                id: saturationSlider
                //value: colorButton.saturation
                onValueChanged: {
                    if (colorButton.saturation !== value  && !colorButton.block) {
                        colorButton.saturation = value
                        colorButton.clicked()
                    }
                }
            }
        }

        Row {
            z: 1
            spacing: 4
            Controls.Label {
                text: "L:"
                width: 16
                color: "#eee"
                elide: Text.ElideRight
                anchors.verticalCenter: parent.verticalCenter
            }
            DoubleSpinBox {
                id: lightnessSlider
                //value: colorButton.lightness
                onValueChanged: {
                    if (colorButton.lightness !== value && !colorButton.block) {
                        colorButton.lightness = value
                        colorButton.clicked()
                    }
                }
            }
        }

        Row {
            z: 0
            spacing: 4
            Controls.Label {
                text: "A:"
                width: 16
                color: "#eee"
                elide: Text.ElideRight
                anchors.verticalCenter: parent.verticalCenter
            }

            DoubleSpinBox {
                id: alphaSlider
                //value: colorButton.alpha
                onValueChanged: {
                    if (colorButton.alpha !== value  && !colorButton.block) {
                        colorButton.alpha = value
                        colorButton.clicked()
                    }
                }
            }
        }

    }
}
