// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.10
import QtQuick.Shapes 1.0

/*!
    \qmltype RectangleItem
    \inqmlmodule QtQuick.Studio.Components
    \since QtQuick.Studio.Components 1.0
    \inherits Shape

    \brief A filled rectangle with an optional border.

    Rectangle items are used to fill areas with solid color or gradients and
    to provide a rectangular border.

    Each Rectangle item is painted using either a solid fill color, specified
    using the \l fillColor property, or a gradient, defined using one of the
    \l ShapeGradient subtypes and set using the \l gradient property.
    If both a color and a gradient are specified, the gradient is used.

    An optional border can be added to a rectangle with its own color and
    thickness by setting the \l strokeColor and \l strokeWidth properties.
    Setting the color to \c transparent creates a border without a fill color.

    Rounded rectangles can be drawn using the \l radius property. The radius
    can also be specified separately for each corner. Because this introduces
    curved edges to the corners of a rectangle, it may be appropriate to set
    the \c antialiasing property that is inherited from \l Item to improve the
    appearance of the rectangle.

    \section2 Example Usage

    You can use the Rectangle component in \QDS to create different kinds of
    rectangles.

    \image studio-rectangle.png

    The QML code looks as follows:

    \code
    RectangleItem {
        id: rectangle
        gradient: RadialGradient {
            focalRadius: 0
            centerY: 38.5
            focalY: 38.5
            centerX: 51.5
            centerRadius: 38.5
            GradientStop {
                position: 0
                color: "#ffffff"
            }

            GradientStop {
                position: 1
                color: "#000000"
            }
            focalX: 51.5
        }
        bottomRightRadius: 0
        topLeftRadius: 0
        strokeColor: "gray"
    }

    RectangleItem {
        id: rectangle1
        gradient: LinearGradient {
            y1: 0
            y2: 77
            x2: 103
            x1: 0
            GradientStop {
                position: 0
                color: "#ffffff"
            }

            GradientStop {
                position: 1
                color: "#000000"
            }
        }
        topRightRadius: 0
        bottomLeftRadius: 0
        strokeColor: "#808080"
    }

    RectangleItem {
        id: rectangle2
        topLeftRadius: 0
        bottomRightRadius: 0
        fillColor: "#d3d3d3"
        strokeColor: "#808080"
    }

    RectangleItem {
        id: rectangle3
        fillColor: "#000000"
        gradient: LinearGradient {
            y1: 0
            y2: 77
            x2: 103
            x1: 0
            GradientStop {
                position: 0
                color: "#000000"
            }

            GradientStop {
                position: 1
                color: "#fdf9f9"
            }
        }
        topRightRadius: 0
        bottomLeftRadius: 0
        strokeColor: "#808080"
    }
    \endcode
*/

Shape {
    id: root
    width: 200
    height: 150

/*!
    The radius used to draw rounded corners.

    The default value is 10.

    If radius is non-zero, the corners will be rounded, otherwise they will
    be sharp. The radius can also be specified separately for each corner by
    using the \l bottomLeftRadius, \l bottomRightRadius, \l topLeftRadius, and
    \l topRightRadius properties.
*/
    property int radius: 10

/*!
    The radius of the top left rectangle corner.
*/
    property int topLeftRadius: root.radius

/*!
    The radius of the bottom left rectangle corner.
*/
    property int bottomLeftRadius: root.radius

/*!
    The radius of the top right rectangle corner.
*/
    property int topRightRadius: root.radius

/*!
    The radius of the bottom right rectangle corner.
*/
    property int bottomRightRadius: root.radius

/*!
    The gradient of the rectangle fill color.

    By default, no gradient is enabled and the value is null. In this case, the
    fill uses a solid color based on the value of \l fillColor.

    When set, \l fillColor is ignored and filling is done using one of the
    \l ShapeGradient subtypes.

    \note The \l Gradient type cannot be used here. Rather, prefer using one of
    the advanced subtypes, like \l LinearGradient.
*/
    property alias gradient: path.fillGradient

/*!
    The style of the rectangle border.

    \value ShapePath.SolidLine
           A solid line. This is the default value.
    \value ShapePath.DashLine
           Dashes separated by a few pixels.
           The \l dashPattern property specifies the dash pattern.

    \sa Qt::PenStyle
*/
    property alias strokeStyle: path.strokeStyle

/*!
    The width of the border of the rectangle.

    The default value is 4.

    A width of 1 creates a thin line. For no line, use a negative value or a
    transparent color.

    \note The width of the rectangle's border does not affect the geometry of
    the rectangle itself or its position relative to other items if anchors are
    used.

    The border is rendered within the rectangle's boundaries.
*/
    property alias strokeWidth: path.strokeWidth

/*!
    The color used to draw the border of the rectangle.

    When set to \c transparent, no line is drawn.

    The default value is \c red.

    \sa QColor
*/
    property alias strokeColor: path.strokeColor

/*!
    The dash pattern of the rectangle border specified as the dashes and the
    gaps between them.

    The dash pattern is specified in units of the pen's width. That is, a dash
    with the length 5 and width 10 is 50 pixels long.

    The default value is (4, 2), meaning a dash of 4 * \l strokeWidth pixels
    followed by a space of 2 * \l strokeWidth pixels.

    \sa QPen::setDashPattern()
*/
    property alias dashPattern: path.dashPattern


    property alias joinStyle: path.joinStyle

/*!
    The rectangle fill color.

    A gradient for the fill can be specified by using \l gradient. If both a
    color and a gradient are specified, the gradient is used.

    When set to \c transparent, no filling occurs.

    The default value is \c white.
*/
    property alias fillColor: path.fillColor

/*!
    The starting point of the dash pattern for the rectangle border.

    The offset is measured in terms of the units used to specify the dash
    pattern. For example, a pattern where each stroke is four units long,
    followed by a gap of two units, will begin with the stroke when drawn
    as a line. However, if the dash offset is set to 4.0, any line drawn
    will begin with the gap. Values of the offset up to 4.0 will cause part
    of the stroke to be drawn first, and values of the offset between 4.0 and
    6.0 will cause the line to begin with part of the gap.

    The default value is 0.

    \sa QPen::setDashOffset()
*/
    property alias dashOffset: path.dashOffset

/*!
    Whether the border corner is beveled.
*/
    property bool bevel: false

/*!
    The bevel of the top left border corner.

    \sa bevel
*/
    property bool topLeftBevel: root.bevel

/*!
    The bevel of the top right border corner.

    \sa bevel
*/
    property bool topRightBevel: root.bevel

/*!
    The bevel of the bottom right border corner.

    \sa bevel
*/
    property bool bottomRightBevel: root.bevel

/*!
    The bevel of the bottom left border corner.

    \sa bevel
*/
    property bool bottomLeftBevel: root.bevel

    layer.enabled: root.antialiasing
    layer.smooth: root.antialiasing
    layer.samples: root.antialiasing ? 4 : 0

/*!
    The border is rendered within the rectangle's boundaries, outside of them,
    or on top of them.
*/
    property int borderMode: 0

    property real borderOffset: {
        if (root.borderMode === 0)
            return root.strokeWidth * 0.5
        if (root.borderMode === 1)
            return 0

        return -root.strokeWidth * 0.5
    }

    Item {
        anchors.fill: parent
        anchors.margins: {
            if (root.borderMode === 0)
                return 0
            if (root.borderMode === 1)
                return -root.strokeWidth * 0.5

            return -root.strokeWidth
        }
    }

    ShapePath {
        id: path

        property int __maxRadius: Math.floor(Math.min(root.width, root.height) / 2)
        property int __topLeftRadius: Math.min(root.topLeftRadius, path.__maxRadius)
        property int __topRightRadius: Math.min(root.topRightRadius, path.__maxRadius)
        property int __bottomRightRadius: Math.min(root.bottomRightRadius, path.__maxRadius)
        property int __bottomLeftRadius: Math.min(root.bottomLeftRadius, path.__maxRadius)

        joinStyle: ShapePath.MiterJoin

        strokeWidth: 4
        strokeColor: "red"

        startX: path.__topLeftRadius + root.borderOffset
        startY: root.borderOffset

        PathLine {
            x: root.width - path.__topRightRadius - root.borderOffset
            y: root.borderOffset
        }

        PathArc {
            x: root.width - root.borderOffset
            y: path.__topRightRadius + root.borderOffset

            radiusX: root.topRightBevel ? 50000 : path.__topRightRadius
            radiusY: root.topRightBevel ? 50000 : path.__topRightRadius
        }

        PathLine {
            x: root.width - root.borderOffset
            y: root.height - path.__bottomRightRadius - root.borderOffset
        }

        PathArc {
            x: root.width - path.__bottomRightRadius - root.borderOffset
            y: root.height - root.borderOffset

            radiusX: root.bottomRightBevel ? 50000 : path.__bottomRightRadius
            radiusY: root.bottomRightBevel ? 50000 : path.__bottomRightRadius
        }

        PathLine {
            x: path.__bottomLeftRadius + root.borderOffset
            y: root.height - root.borderOffset
        }

        PathArc {
            x: root.borderOffset
            y: root.height - path.__bottomLeftRadius - root.borderOffset

            radiusX: root.bottomLeftBevel ? 50000 : path.__bottomLeftRadius
            radiusY: root.bottomLeftBevel ? 50000 : path.__bottomLeftRadius
        }

        PathLine {
            x: root.borderOffset
            y: path.__topLeftRadius + root.borderOffset
        }

        PathArc {
            x: path.__topLeftRadius + root.borderOffset
            y: root.borderOffset

            radiusX: root.topLeftBevel ? 50000 : path.__topLeftRadius
            radiusY: root.topLeftBevel ? 50000 : path.__topLeftRadius
        }
    }
}
