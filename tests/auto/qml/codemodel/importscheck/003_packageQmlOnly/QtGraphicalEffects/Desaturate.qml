// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0
import "private"

/*!
    \qmltype Desaturate
    \inqmlmodule QtGraphicalEffects 1.0
    \since QtGraphicalEffects 1.0
    \inherits QtQuick2::Item
    \ingroup qtgraphicaleffects-color
    \brief Reduces the saturation of the colors.

   Desaturated pixel values are calculated as averages of the original RGB
   component values of the source item.

    \table
    \header
        \li Source
        \li Effect applied
    \row
        \li \image Original_bug.png
        \li \image Desaturate_bug.png
    \endtable

    \section1 Example

    The following example shows how to apply the effect.
    \snippet Desaturate-example.qml example

*/
Item {
    id: rootItem

    /*!
        This property defines the source item that provides the source pixels to
        the effect.

        \note It is not supported to let the effect include itself, for
        instance by setting source to the effect's parent.
    */
    property variant source

    /*!
        This property defines how much the source colors are desaturated.

        The value ranges from 0.0 (no change) to 1.0 (desaturated). By default,
        the property is set to \c 0.0 (no chnage).

        \table
        \header
        \li Output examples with different desaturation values
        \li
        \li
        \row
            \li \image Desaturate_desaturation1.png
            \li \image Desaturate_desaturation2.png
            \li \image Desaturate_desaturation3.png
        \row
            \li \b { desaturation: 0.0 }
            \li \b { desaturation: 0.5 }
            \li \b { desaturation: 1.0 }
        \endtable
    */
    property real desaturation: 0.0

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
        property real desaturation: rootItem.desaturation

        anchors.fill: parent

        fragmentShader: "
            varying highp vec2 qt_TexCoord0;
            uniform highp float qt_Opacity;
            uniform lowp sampler2D source;
            uniform highp float desaturation;
            void main(void) {
                lowp vec4 textureColor = texture2D(source, qt_TexCoord0.st);
                lowp float grayColor = (textureColor.r + textureColor.g + textureColor.b) / 3.0;
                gl_FragColor = mix(textureColor, vec4(vec3(grayColor), textureColor.a), desaturation) * qt_Opacity;
            }
        "
    }
}
