// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0
import "private"

/*!
    \qmltype ColorOverlay
    \inqmlmodule QtGraphicalEffects 1.0
    \since QtGraphicalEffects 1.0
    \inherits QtQuick2::Item
    \ingroup qtgraphicaleffects-color
    \brief Alters the colors of the source item by applying an overlay color.

    The effect is similar to what happens when a colorized glass is put on top
    of a grayscale image. The color for the overlay is given in the RGBA format.

    \table
    \header
        \li Source
        \li Effect applied
    \row
        \li \image Original_butterfly.png
        \li \image ColorOverlay_butterfly.png
    \endtable

    \section1 Example

    The following example shows how to apply the effect.
    \snippet ColorOverlay-example.qml example

*/
Item {
    id: rootItem

    /*!
        This property defines the source item that provides the source pixels
        for the effect.

        \note It is not supported to let the effect include itself, for
        instance by setting source to the effect's parent.
    */
    property variant source

    /*!
        This property defines the RGBA color value which is used to colorize the
        source.

        By default, the property is set to \c "transparent".

        \table
        \header
        \li Output examples with different color values
        \li
        \li
        \row
            \li \image ColorOverlay_color1.png
            \li \image ColorOverlay_color2.png
            \li \image ColorOverlay_color3.png
        \row
            \li \b { color: #80ff0000 }
            \li \b { color: #8000ff00 }
            \li \b { color: #800000ff }
        \endtable

    */
    property color color: "transparent"

    /*!
        This property allows the effect output pixels to be cached in order to
        improve the rendering performance.

        Every time the source or effect properties are changed, the pixels in
        the cache must be updated. Memory consumption is increased, because an
        extra buffer of memory is required for storing the effect output.

        It is recommended to disable the cache when the source or the effect
        properties are animated.

        By default, the property is set to \c false.

    */
    property bool cached: false

    SourceProxy {
        id: sourceProxy
        input: rootItem.source
    }

    ShaderEffectSource {
        id: cacheItem
        anchors.fill: parent
        visible: rootItem.cached
        smooth: true
        sourceItem: shaderItem
        live: true
        hideSource: visible
    }

    ShaderEffect {
        id: shaderItem
        property variant source: sourceProxy.output
        property color color: rootItem.color

        anchors.fill: parent

        fragmentShader: "
            varying mediump vec2 qt_TexCoord0;
            uniform highp float qt_Opacity;
            uniform lowp sampler2D source;
            uniform highp vec4 color;
            void main() {
                highp vec4 pixelColor = texture2D(source, qt_TexCoord0);
                gl_FragColor = vec4(mix(pixelColor.rgb/max(pixelColor.a, 0.00390625), color.rgb/max(color.a, 0.00390625), color.a) * pixelColor.a, pixelColor.a) * qt_Opacity;
            }
        "
    }
}
