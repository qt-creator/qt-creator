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

    enum Mode {
        HSVA,
        RGBA,
        HSLA
    }

    property int mode: ColorPicker.Mode.HSVA
    property color color

    property real hue: 0
    property real saturationHSL: 0
    property real saturationHSV: 0
    property real lightness: 0
    property real value: 0

    property real alpha: 1

    property bool achromatic: false

    property int sliderMargins: 6
    property bool block: false

    signal updateColor
    signal rightMouseButtonClicked
    signal colorInvalidated

    spacing: 10

    onModeChanged: {
        switch (root.mode) {
        case ColorPicker.Mode.RGBA:
            root.color = Qt.rgba(root.color.r, root.color.g, root.color.b, root.alpha)
            break
        case ColorPicker.Mode.HSLA:
            root.color = Qt.hsla(root.hue, root.saturationHSL, root.lightness, root.alpha)
            break
        case ColorPicker.Mode.HSVA:
        default:
            root.color = Qt.hsva(root.hue, root.saturationHSV, root.value, root.alpha)
            break
        }

        gradientOverlay.requestPaint()
    }

    onHueChanged: {
        if (root.mode === ColorPicker.Mode.HSLA)
            root.color.hslHue = root.hue
        else
            root.color.hsvHue = root.hue
    }
    onSaturationHSLChanged: {
        root.color.hslSaturation = root.saturationHSL
        invalidateColor()
    }
    onSaturationHSVChanged: {
        root.color.hsvSaturation = root.saturationHSV
    }
    onLightnessChanged: {
        root.color.hslLightness = root.lightness
    }
    onValueChanged: {
        root.color.hsvValue = root.value
    }
    onAlphaChanged: invalidateColor()
    onColorChanged: invalidateColor()

    function invalidateColor() {
        if (root.block)
            return

        root.block = true

        if (root.color.hsvSaturation > 0.0
                && root.color.hsvValue > 0.0
                && root.color.hsvHue !== -1.0)
            root.hue = root.color.hsvHue

        if (root.color.hslSaturation > 0.0
                && root.color.hslLightness > 0.0
                && root.color.hslHue !== -1.0)
            root.hue = root.color.hslHue

        if (root.color.hslLightness !== 0.0 && root.color.hslLightness !== 1.0 && !root.achromatic)
            root.saturationHSL = root.color.hslSaturation

        if (root.color.hsvValue !== 0.0 && root.color.hsvValue !== 1.0 && !root.achromatic)
            root.saturationHSV = root.color.hsvSaturation

        root.lightness = root.color.hslLightness
        root.value = root.color.hsvValue

        if (root.color.hslLightness === 0.0 || root.color.hslLightness === 1.0
                || root.color.hsvValue === 0.0 || root.color.hsvValue === 1.0
                || root.color.hsvHue === -1.0 || root.color.hslHue === -1.0)
            root.achromatic = true
        else
            root.achromatic = false

        if (root.mode === ColorPicker.Mode.HSLA)
            root.color = Qt.hsla(root.hue, root.saturationHSL, root.lightness, root.alpha)
        else
            root.color = Qt.hsva(root.hue, root.saturationHSV, root.value, root.alpha)

        luminanceSlider.value = (1.0 - root.value)
        hueSlider.value = root.hue
        opacitySlider.value = (1.0 - root.alpha)

        root.colorInvalidated()

        root.block = false
    }

    function drawHSVA(ctx) {
        for (var row = 0; row < gradientOverlay.height; row++) {
            var gradient = ctx.createLinearGradient(0, 0, gradientOverlay.width, 0)
            var v = Math.abs(row - gradientOverlay.height) / gradientOverlay.height

            gradient.addColorStop(0, Qt.hsva(root.hue, 0, v, 1))
            gradient.addColorStop(1, Qt.hsva(root.hue, 1, v, 1))

            ctx.fillStyle = gradient
            ctx.fillRect(0, row, gradientOverlay.width, 1)
        }
    }

    function drawRGBA(ctx) {
        var gradient = ctx.createLinearGradient(0, 0, gradientOverlay.width, 0)
        gradient.addColorStop(0.000, Qt.rgba(1, 0, 0, 1))
        gradient.addColorStop(0.167, Qt.rgba(1, 1, 0, 1))
        gradient.addColorStop(0.333, Qt.rgba(0, 1, 0, 1))
        gradient.addColorStop(0.500, Qt.rgba(0, 1, 1, 1))
        gradient.addColorStop(0.667, Qt.rgba(0, 0, 1, 1))
        gradient.addColorStop(0.833, Qt.rgba(1, 0, 1, 1))
        gradient.addColorStop(1.000, Qt.rgba(1, 0, 0, 1))

        ctx.fillStyle = gradient
        ctx.fillRect(0, 0, gradientOverlay.width, gradientOverlay.height)

        gradient = ctx.createLinearGradient(0, 0, 0, gradientOverlay.height)
        gradient.addColorStop(0.000, Qt.rgba(0, 0, 0, 0))
        gradient.addColorStop(1.000, Qt.rgba(1, 1, 1, 1))

        ctx.fillStyle = gradient
        ctx.fillRect(0, 0, gradientOverlay.width, gradientOverlay.height)
    }

    function drawHSLA(ctx) {
        for (var row = 0; row < gradientOverlay.height; row++) {
            var gradient = ctx.createLinearGradient(0, 0, gradientOverlay.width, 0)
            var l = Math.abs(row - gradientOverlay.height) / gradientOverlay.height

            gradient.addColorStop(0, Qt.hsla(root.hue, 0, l, 1))
            gradient.addColorStop(1, Qt.hsla(root.hue, 1, l, 1))

            ctx.fillStyle = gradient
            ctx.fillRect(0, row, gradientOverlay.width, 1)
        }
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

                anchors.fill: parent
                opacity: root.color.a

                Connections {
                    target: root
                    function onHueChanged() { gradientOverlay.requestPaint() }
                }

                onPaint: {
                    var ctx = gradientOverlay.getContext('2d')
                    ctx.save()
                    ctx.clearRect(0, 0, gradientOverlay.width, gradientOverlay.height)

                    switch (root.mode) {
                    case ColorPicker.Mode.RGBA:
                        root.drawRGBA(ctx)
                        break
                    case ColorPicker.Mode.HSLA:
                        root.drawHSLA(ctx)
                        break
                    case ColorPicker.Mode.HSVA:
                    default:
                        root.drawHSVA(ctx)
                        break
                    }

                    ctx.restore()
                }
            }

            Canvas {
                id: pickerCross

                property color strokeStyle: "lightGray"

                opacity: 0.8
                anchors.fill: parent
                antialiasing: true

                Connections {
                    target: root
                    function onColorInvalidated() { pickerCross.requestPaint() }
                    function onColorChanged() { pickerCross.requestPaint() }
                    function onModeChanged() { pickerCross.requestPaint() }
                }

                onPaint: {
                    var ctx = pickerCross.getContext('2d')
                    ctx.save()
                    ctx.clearRect(0, 0, pickerCross.width, pickerCross.height)

                    var yy, xx = 0

                    switch (root.mode) {
                    case ColorPicker.Mode.RGBA:
                        yy = pickerCross.height - root.saturationHSV * pickerCross.height
                        xx = root.hue * pickerCross.width
                        break
                    case ColorPicker.Mode.HSLA:
                        yy = pickerCross.height - root.lightness * pickerCross.height
                        xx = root.saturationHSL * pickerCross.width
                        break
                    case ColorPicker.Mode.HSVA:
                    default:
                        yy = pickerCross.height - root.value * pickerCross.height
                        xx = root.saturationHSV * pickerCross.width
                        break
                    }

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
                id: mouseArea

                anchors.fill: parent
                preventStealing: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton

                onPositionChanged: function(mouse) {
                    if (mouseArea.pressed && mouse.buttons === Qt.LeftButton) {
                        var xx = Math.max(0, Math.min(mouse.x, parent.width))
                        var yy = Math.max(0, Math.min(mouse.y, parent.height))

                        switch (root.mode) {
                        case ColorPicker.Mode.RGBA:
                            root.saturationHSV = 1.0 - yy / parent.height
                            root.hue = xx / parent.width
                            break
                        case ColorPicker.Mode.HSLA:
                            root.saturationHSL = xx / parent.width
                            root.lightness = 1.0 - yy / parent.height
                            break
                        case ColorPicker.Mode.HSVA:
                        default:
                            root.saturationHSV = xx / parent.width
                            root.value = 1.0 - yy / parent.height
                            break
                        }
                    }
                }
                onPressed: function(mouse) {
                    if (mouse.button === Qt.LeftButton)
                        mouseArea.positionChanged(mouse)
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
        visible: root.mode !== ColorPicker.Mode.RGBA
        width: parent.width
        onValueChanged: {
            if (root.hue !== hueSlider.value)
                root.hue = hueSlider.value
        }
        onClicked: root.updateColor()
    }

    LuminanceSlider {
        id: luminanceSlider
        visible: root.mode === ColorPicker.Mode.RGBA
        width: parent.width
        color: Qt.hsva(root.hue, root.color.hsvSaturation, 1, 1)
        onValueChanged: {
            if (root.value !== luminanceSlider.value)
                root.value = (1.0 - luminanceSlider.value)
        }
        onClicked: root.updateColor()
    }

    OpacitySlider {
        id: opacitySlider
        width: parent.width
        color: Qt.rgba(root.color.r, root.color.g, root.color.b, 1)
        onValueChanged: {
            if (root.alpha !== opacitySlider.value)
                root.alpha = (1.0 - opacitySlider.value)
        }
        onClicked: root.updateColor()
    }
}
