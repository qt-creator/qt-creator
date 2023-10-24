// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    property color color: "#303091"

    property real red: 0
    property real green: 0
    property real blue: 0

    property real hue: 0
    property real saturationHSL: 0.5
    property real saturationHSV: 0.5
    property real lightness: 0.5
    property real value: 0.5

    property real alpha: 1

    property int sliderMargins: 6
    property bool block: false

    signal updateColor
    signal rightMouseButtonClicked
    signal colorInvalidated

    spacing: 10

    onColorChanged: {
        if (root.block)
            return

        switch (root.mode) {
        case ColorPicker.Mode.RGBA:
            root.red = root.color.r
            root.green = root.color.g
            root.blue = root.color.b
            root.alpha = root.color.a

            break
        case ColorPicker.Mode.HSLA:
            if (root.color.hslHue !== -1)
                root.hue = root.color.hslHue

            root.saturationHSL = root.color.hslSaturation
            root.lightness = root.color.hslLightness
            root.alpha = root.color.a

            break
        case ColorPicker.Mode.HSVA:
        default:
            if (root.color.hsvHue !== -1)
                root.hue = root.color.hsvHue

            root.saturationHSV = root.color.hsvSaturation
            root.value = root.color.hsvValue
            root.alpha = root.color.a

            break
        }

        root.invalidateColor()
    }

    function invalidateColor() {
        if (root.block)
            return

        root.block = true

        switch (root.mode) {
        case ColorPicker.Mode.RGBA:
            root.color = Qt.rgba(root.red, root.green, root.blue, root.alpha)
            // Set HSVA and HSLA
            if (root.color.hsvHue !== -1)
                root.hue = root.color.hsvHue // doesn't matter if hsvHue or hslHue

            if (root.color.hslLightness !== 0.0 && root.color.hslLightness !== 1.0)
                root.saturationHSL = root.color.hslSaturation

            if (root.color.hsvValue !== 0.0)
                root.saturationHSV = root.color.hsvSaturation

            root.lightness = root.color.hslLightness
            root.value = root.color.hsvValue
            break
        case ColorPicker.Mode.HSLA:
            root.color = Qt.hsla(root.hue, root.saturationHSL, root.lightness, root.alpha)
            // Set RGBA and HSVA
            root.red = root.color.r
            root.green = root.color.g
            root.blue = root.color.b

            if (root.color.hsvValue !== 0.0)
                root.saturationHSV = root.color.hsvSaturation

            root.value = root.color.hsvValue
            break
        case ColorPicker.Mode.HSVA:
        default:
            root.color = Qt.hsva(root.hue, root.saturationHSV, root.value, root.alpha)
            // Set RGBA and HSLA
            root.red = root.color.r
            root.green = root.color.g
            root.blue = root.color.b

            if (root.color.hslLightness !== 0.0 && root.color.hslLightness !== 1.0)
                root.saturationHSL = root.color.hslSaturation

            root.lightness = root.color.hslLightness
            break
        }

        luminanceSlider.value = (1.0 - root.value)
        hueSlider.value = root.hue
        opacitySlider.value = (1.0 - root.alpha)

        root.colorInvalidated()

        root.block = false
    }

    function drawHSVA(ctx, width, height, hue) {
        for (var row = 0; row < height; row++) {
            var gradient = ctx.createLinearGradient(0, 0, width, 0)
            var v = Math.abs(row - height) / height

            gradient.addColorStop(0, Qt.hsva(hue, 0, v, 1))
            gradient.addColorStop(1, Qt.hsva(hue, 1, v, 1))

            ctx.fillStyle = gradient
            ctx.fillRect(0, row, width, 1)
        }
    }

    function drawRGBA(ctx, width, height) {
        var gradient = ctx.createLinearGradient(0, 0, width, 0)
        gradient.addColorStop(0.000, Qt.rgba(1, 0, 0, 1))
        gradient.addColorStop(0.167, Qt.rgba(1, 1, 0, 1))
        gradient.addColorStop(0.333, Qt.rgba(0, 1, 0, 1))
        gradient.addColorStop(0.500, Qt.rgba(0, 1, 1, 1))
        gradient.addColorStop(0.667, Qt.rgba(0, 0, 1, 1))
        gradient.addColorStop(0.833, Qt.rgba(1, 0, 1, 1))
        gradient.addColorStop(1.000, Qt.rgba(1, 0, 0, 1))

        ctx.fillStyle = gradient
        ctx.fillRect(0, 0, width, height)

        gradient = ctx.createLinearGradient(0, 0, 0, height)
        gradient.addColorStop(0.000, Qt.rgba(0, 0, 0, 0))
        gradient.addColorStop(1.000, Qt.rgba(1, 1, 1, 1))

        ctx.fillStyle = gradient
        ctx.fillRect(0, 0, width, height)
    }

    function drawHSLA(ctx, width, height, hue) {
        for (var row = 0; row < height; row++) {
            var gradient = ctx.createLinearGradient(0, 0, width, 0)
            var l = Math.abs(row - height) / height

            gradient.addColorStop(0, Qt.hsla(hue, 0, l, 1))
            gradient.addColorStop(1, Qt.hsla(hue, 1, l, 1))

            ctx.fillStyle = gradient
            ctx.fillRect(0, row, width, 1)
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

            source: "qrc:/navigator/icon/checkers.png"
            fillMode: Image.Tile

            // Note: We smoothscale the shader from a smaller version to improve performance
            Canvas {
                id: gradientOverlay

                anchors.fill: parent
                opacity: root.alpha

                Connections {
                    target: root
                    function onHueChanged() { gradientOverlay.requestPaint() }
                    function onModeChanged() { gradientOverlay.requestPaint() }
                }

                onPaint: {
                    var ctx = gradientOverlay.getContext('2d')
                    ctx.save()
                    ctx.clearRect(0, 0, gradientOverlay.width, gradientOverlay.height)

                    switch (root.mode) {
                    case ColorPicker.Mode.RGBA:
                        root.drawRGBA(ctx, gradientOverlay.width, gradientOverlay.height)
                        break
                    case ColorPicker.Mode.HSLA:
                        root.drawHSLA(ctx, gradientOverlay.width, gradientOverlay.height, root.hue)
                        break
                    case ColorPicker.Mode.HSVA:
                    default:
                        root.drawHSVA(ctx, gradientOverlay.width, gradientOverlay.height, root.hue)
                        break
                    }

                    ctx.restore()
                }
            }

            Canvas {
                id: pickerCross

                property color strokeStyle: "lightGray"
                property string loadImageUrl: "qrc:/navigator/icon/checkers.png"
                property int radius: 10

                Component.onCompleted: pickerCross.loadImage(pickerCross.loadImageUrl)
                onImageLoaded: pickerCross.requestPaint()

                anchors.fill: parent
                anchors.margins: -pickerCross.radius
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

                    var normX, normY = 0

                    switch (root.mode) {
                    case ColorPicker.Mode.RGBA:
                        normX = root.hue
                        normY = 1.0 - root.saturationHSV
                        break
                    case ColorPicker.Mode.HSLA:
                        normX = root.saturationHSL
                        normY = 1.0 - root.lightness
                        break
                    case ColorPicker.Mode.HSVA:
                    default:
                        normX = root.saturationHSV
                        normY = 1.0 - root.value
                        break
                    }

                    var width = pickerCross.width - pickerCross.radius * 2
                    var height = pickerCross.height - pickerCross.radius * 2

                    var x = normX * width
                    var y = normY * height

                    var centerX = pickerCross.radius + x
                    var centerY = pickerCross.radius + y

                    // Draw checkerboard
                    if (isImageLoaded(pickerCross.loadImageUrl)) {
                        ctx.beginPath()
                        ctx.arc(centerX, centerY, pickerCross.radius, 0, 2 * Math.PI)
                        ctx.clip()

                        var pattern = context.createPattern(pickerCross.loadImageUrl, 'repeat')
                        context.fillStyle = pattern

                        ctx.fillRect(x, y, pickerCross.radius * 2, pickerCross.radius * 2)
                    }

                    // Draw current color
                    if (root.mode === ColorPicker.Mode.RGBA)
                        ctx.fillStyle = Qt.hsva(root.hue, root.saturationHSV, 1, root.alpha)
                    else
                        ctx.fillStyle = root.color

                    ctx.fillRect(x, y, pickerCross.radius * 2, pickerCross.radius * 2)

                    // Draw black and white circle
                    ctx.lineWidth = 2
                    ctx.strokeStyle = "black"
                    ctx.beginPath()
                    ctx.arc(centerX, centerY, pickerCross.radius, 0, 2 * Math.PI)
                    ctx.stroke()

                    ctx.strokeStyle = "white"
                    ctx.beginPath()
                    ctx.arc(centerX, centerY, pickerCross.radius - 2, 0, 2 * Math.PI)
                    ctx.stroke()

                    ctx.restore()
                }
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                anchors.margins: -pickerCross.radius
                preventStealing: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton

                onPositionChanged: function(mouse) {
                    if (mouseArea.pressed && mouse.buttons === Qt.LeftButton) {
                        // Generate color values from mouse position

                        // Clip/limit to margin
                        var x = Math.max(0, Math.min(mouse.x - pickerCross.radius, parent.width))
                        var y = Math.max(0, Math.min(mouse.y - pickerCross.radius, parent.height))

                        var normX = x / parent.width
                        var normY = y / parent.height

                        switch (root.mode) {
                        case ColorPicker.Mode.RGBA:
                            var tmpColor = Qt.hsva(normX, // hue
                                                   1.0 - normY, // saturation
                                                   root.value,
                                                   root.alpha)

                            root.hue = normX
                            root.saturationHSV = 1.0 - normY

                            root.red = tmpColor.r
                            root.green = tmpColor.g
                            root.blue = tmpColor.b
                            break
                        case ColorPicker.Mode.HSLA:
                            root.saturationHSL = normX
                            root.lightness = 1.0 - normY
                            break
                        case ColorPicker.Mode.HSVA:
                        default:
                            root.saturationHSV = normX
                            root.value = 1.0 - normY
                            break
                        }

                        root.invalidateColor()
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
        onMoved: {
            if (root.hue !== hueSlider.value)
                root.hue = hueSlider.value

            root.invalidateColor()
        }
        onClicked: root.updateColor()
    }

    LuminanceSlider {
        id: luminanceSlider
        visible: root.mode === ColorPicker.Mode.RGBA
        width: parent.width
        color: Qt.hsva(root.hue, root.saturationHSV, root.value, root.alpha)
        onMoved: {
            if (root.value !== (1.0 - luminanceSlider.value))
                root.value = (1.0 - luminanceSlider.value)

            var tmpColor = Qt.hsva(root.hue,
                                   root.saturationHSV,
                                   root.value,
                                   root.alpha)

            root.red = tmpColor.r
            root.green = tmpColor.g
            root.blue = tmpColor.b

            root.invalidateColor()
        }
        onClicked: root.updateColor()
    }

    OpacitySlider {
        id: opacitySlider
        width: parent.width
        color: root.color
        onMoved: {
            if (root.alpha !== (1.0 - opacitySlider.value))
                root.alpha = (1.0 - opacitySlider.value)

            root.invalidateColor()
        }
        onClicked: root.updateColor()
    }
}
