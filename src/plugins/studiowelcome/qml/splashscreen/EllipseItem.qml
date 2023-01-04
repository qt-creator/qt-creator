// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.10
import QtQuick.Shapes 1.0

/*!
    \qmltype EllipseItem
    \inqmlmodule QtQuick.Studio.Components
    \since QtQuick.Studio.Components 1.0
    \inherits Shape

    \brief A filled ellipse with an optional border.
*/

Shape {
    id: root
    width: 200
    height: 150

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

        joinStyle: ShapePath.MiterJoin
        strokeWidth: 4
        strokeColor: "red"
        startX: root.width * 0.5
        startY: root.borderOffset

        PathArc {
            x: path.startX
            y: root.height - root.borderOffset
            radiusX: root.width * 0.5 - root.borderOffset
            radiusY: root.height * 0.5 - root.borderOffset
            useLargeArc: true
        }

        PathArc {
            x: path.startX
            y: path.startY
            radiusX: root.width * 0.5 - root.borderOffset
            radiusY: root.height * 0.5 - root.borderOffset
            useLargeArc: true
        }

    }
}
