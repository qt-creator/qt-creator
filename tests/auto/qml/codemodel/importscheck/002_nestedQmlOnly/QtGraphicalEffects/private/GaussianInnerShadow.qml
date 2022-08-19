// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0

Item {
    id: rootItem
    property variant source
    property real radius: 0.0
    property int maximumRadius: 0
    property real horizontalOffset: 0
    property real verticalOffset: 0
    property real spread: 0
    property color color: "black"
    property bool cached: false

    SourceProxy {
        id: sourceProxy
        input: rootItem.source
    }

    ShaderEffectSource {
        id: cacheItem
        anchors.fill: shaderItem
        visible: rootItem.cached
        smooth: true
        sourceItem: shaderItem
        live: true
        hideSource: visible
    }

    ShaderEffect{
        id: shadowItem
        anchors.fill: parent

        property variant original: sourceProxy.output
        property color color: rootItem.color
        property real horizontalOffset: rootItem.horizontalOffset / rootItem.width
        property real verticalOffset: rootItem.verticalOffset / rootItem.height

        visible: false
        fragmentShader: "
            uniform highp sampler2D original;
            uniform lowp float qt_Opacity;
            uniform lowp vec4 color;
            uniform highp float horizontalOffset;
            uniform highp float verticalOffset;
            varying highp vec2 qt_TexCoord0;

            void main(void) {
                highp vec2 pos = qt_TexCoord0 - vec2(horizontalOffset, verticalOffset);
                lowp float ea = step(0.0, pos.x) * step(0.0, pos.y) * step(pos.x, 1.0) * step(pos.y, 1.0);
                lowp float eb = 1.0 - ea;
                gl_FragColor = eb * color + ea * color * (1.0 - texture2D(original, pos).a) * qt_Opacity;
            }
        "
    }

    GaussianDirectionalBlur {
        id: blurItem
        anchors.fill: parent
        horizontalStep: 0.0
        verticalStep: 1.0 / parent.height
        source: horizontalBlur
        radius: rootItem.radius
        maximumRadius: rootItem.maximumRadius
        visible: false
    }

    GaussianDirectionalBlur {
        id: horizontalBlur
        width: transparentBorder ? parent.width + 2 * maximumRadius : parent.width
        height: parent.height
        horizontalStep: 1.0 / parent.width
        verticalStep: 0.0
        source: shadowItem
        radius: rootItem.radius
        maximumRadius: rootItem.maximumRadius
        visible: false
    }

    ShaderEffectSource {
        id: blurredSource
        sourceItem: blurItem
        live: true
        smooth: true
    }

    ShaderEffect {
        id: shaderItem
        anchors.fill: parent

        property variant original: sourceProxy.output
        property variant shadow: blurredSource
        property real spread: 1.0 - (rootItem.spread * 0.98)
        property color color: rootItem.color

        fragmentShader: "
            uniform highp sampler2D original;
            uniform highp sampler2D shadow;
            uniform lowp float qt_Opacity;
            uniform highp float spread;
            uniform lowp vec4 color;
            varying highp vec2 qt_TexCoord0;

            highp float linearstep(highp float e0, highp float e1, highp float x) {
                return clamp((x - e0) / (e1 - e0), 0.0, 1.0);
            }

            void main(void) {
                lowp vec4 originalColor = texture2D(original, qt_TexCoord0);
                lowp vec4 shadowColor = texture2D(shadow, qt_TexCoord0);
                shadowColor.rgb = mix(originalColor.rgb, color.rgb * originalColor.a, linearstep(0.0, spread, shadowColor.a));
                gl_FragColor = vec4(shadowColor.rgb, originalColor.a) * originalColor.a * qt_Opacity;
            }
        "
    }
}
