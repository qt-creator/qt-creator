// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0
import "private"

/*!
    \qmltype RectangularGlow
    \inqmlmodule QtGraphicalEffects 1.0
    \since QtGraphicalEffects 1.0
    \inherits QtQuick2::Item
    \ingroup qtgraphicaleffects-glow
    \brief Generates a blurred and colorized rectangle, which gives
    the impression that the source is glowing.

    This effect is intended to have good performance. The shape of the glow is
    limited to a rectangle with a custom corner radius. For situations where
    custom shapes are required, consider \l {QtGraphicalEffects1::Glow} {Glow}
    effect.

    \table
    \header
        \li Effect applied
    \row
        \li \image RectangularGlow_applied.png
    \endtable

    \section1 Example

    The following example shows how to apply the effect.
    \snippet RectangularGlow-example.qml example
*/
Item {
    id: rootItem

    /*!
        This property defines how many pixels outside the item area are reached
        by the glow.

        The value ranges from 0.0 (no glow) to inf (infinite glow). By default,
        the property is set to \c 0.0.

        \table
        \header
        \li Output examples with different glowRadius values
        \li
        \li
        \row
            \li \image RectangularGlow_glowRadius1.png
            \li \image RectangularGlow_glowRadius2.png
            \li \image RectangularGlow_glowRadius3.png
        \row
            \li \b { glowRadius: 10 }
            \li \b { glowRadius: 20 }
            \li \b { glowRadius: 40 }
        \row
            \li \l spread: 0
            \li \l spread: 0
            \li \l spread: 0
        \row
            \li \l color: #ffffff
            \li \l color: #ffffff
            \li \l color: #ffffff
        \row
            \li \l cornerRadius: 25
            \li \l cornerRadius: 25
            \li \l cornerRadius: 25
        \endtable

    */
    property real glowRadius: 0.0

    /*!
        This property defines how large part of the glow color is strenghtened
        near the source edges.

        The value ranges from 0.0 (no strenght increase) to 1.0 (maximum
        strenght increase). By default, the property is set to \c 0.0.

        \table
        \header
        \li Output examples with different spread values
        \li
        \li
        \row
            \li \image RectangularGlow_spread1.png
            \li \image RectangularGlow_spread2.png
            \li \image RectangularGlow_spread3.png
        \row
            \li \b { spread: 0.0 }
            \li \b { spread: 0.5 }
            \li \b { spread: 1.0 }
        \row
            \li \l glowRadius: 20
            \li \l glowRadius: 20
            \li \l glowRadius: 20
        \row
            \li \l color: #ffffff
            \li \l color: #ffffff
            \li \l color: #ffffff
        \row
            \li \l cornerRadius: 25
            \li \l cornerRadius: 25
            \li \l cornerRadius: 25
        \endtable
    */
    property real spread: 0.0

    /*!
        This property defines the RGBA color value which is used for the glow.

        By default, the property is set to \c "white".

        \table
        \header
        \li Output examples with different color values
        \li
        \li
        \row
            \li \image RectangularGlow_color1.png
            \li \image RectangularGlow_color2.png
            \li \image RectangularGlow_color3.png
        \row
            \li \b { color: #ffffff }
            \li \b { color: #55ff55 }
            \li \b { color: #5555ff }
        \row
            \li \l glowRadius: 20
            \li \l glowRadius: 20
            \li \l glowRadius: 20
        \row
            \li \l spread: 0
            \li \l spread: 0
            \li \l spread: 0
        \row
            \li \l cornerRadius: 25
            \li \l cornerRadius: 25
            \li \l cornerRadius: 25
        \endtable
    */
    property color color: "white"

    /*!
        This property defines the corner radius that is used to draw a glow with
        rounded corners.

        The value ranges from 0.0 to half of the effective width or height of
        the glow, whichever is smaller. This can be calculated with: \c{
        min(width, height) / 2.0 + glowRadius}

        By default, the property is bound to glowRadius property. The glow
        behaves as if the rectangle was blurred when adjusting the glowRadius
        property.

        \table
        \header
        \li Output examples with different cornerRadius values
        \li
        \li
        \row
            \li \image RectangularGlow_cornerRadius1.png
            \li \image RectangularGlow_cornerRadius2.png
            \li \image RectangularGlow_cornerRadius3.png
        \row
            \li \b { cornerRadius: 0 }
            \li \b { cornerRadius: 25 }
            \li \b { cornerRadius: 50 }
        \row
            \li \l glowRadius: 20
            \li \l glowRadius: 20
            \li \l glowRadius: 20
        \row
            \li \l spread: 0
            \li \l spread: 0
            \li \l spread: 0
        \row
            \li \l color: #ffffff
            \li \l color: #ffffff
            \li \l color: #ffffff
        \endtable
    */
    property real cornerRadius: glowRadius

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

    ShaderEffectSource {
         id: cacheItem
         anchors.fill: shaderItem
         visible: rootItem.cached
         smooth: true
         sourceItem: shaderItem
         live: true
         hideSource: visible
     }

    ShaderEffect {
        id: shaderItem

        x: (parent.width - width) / 2.0
        y: (parent.height - height) / 2.0
        width: parent.width + rootItem.glowRadius * 2 + cornerRadius * 2
        height: parent.height + rootItem.glowRadius * 2 + cornerRadius * 2

        function clampedCornerRadius() {
            var maxCornerRadius = Math.min(rootItem.width, rootItem.height) / 2 + glowRadius;
            return Math.max(0, Math.min(rootItem.cornerRadius, maxCornerRadius))
        }

        property color color: rootItem.color
        property real inverseSpread: 1.0 - rootItem.spread
        property real relativeSizeX: ((inverseSpread * inverseSpread) * rootItem.glowRadius + cornerRadius * 2.0) / width
        property real relativeSizeY: relativeSizeX * (width / height)
        property real spread: rootItem.spread / 2.0
        property real cornerRadius: clampedCornerRadius()

        fragmentShader: "
            uniform highp float qt_Opacity;
            uniform mediump float relativeSizeX;
            uniform mediump float relativeSizeY;
            uniform mediump float spread;
            uniform lowp vec4 color;
            varying highp vec2 qt_TexCoord0;

            highp float linearstep(highp float e0, highp float e1, highp float x) {
                return clamp((x - e0) / (e1 - e0), 0.0, 1.0);
            }

            void main() {
                lowp float alpha =
                    smoothstep(0.0, relativeSizeX, 0.5 - abs(0.5 - qt_TexCoord0.x)) *
                    smoothstep(0.0, relativeSizeY, 0.5 - abs(0.5 - qt_TexCoord0.y));

                highp float spreadMultiplier = linearstep(spread, 1.0 - spread, alpha);
                gl_FragColor = color * qt_Opacity * spreadMultiplier * spreadMultiplier;
            }
        "
    }
}
