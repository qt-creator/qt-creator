/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#pragma once

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QPolygonF;
class QLineF;
class QPointF;
class QRectF;
class QSizeF;
QT_END_NAMESPACE

namespace qmt {

class GeometryUtilities
{
    GeometryUtilities() = delete;

public:
    enum Side {
        SideUnspecified,
        SideTop,
        SideBottom,
        SideLeft,
        SideRight
    };

    static QLineF stretch(const QLineF &line, double p1Extension, double p2Extension);
    static bool intersect(const QPolygonF &polygon, const QLineF &line,
                          QPointF *intersectionPoint, QLineF *intersectionLine = nullptr);
    static bool placeRectAtLine(const QRectF &rect, const QLineF &line, double lineOffset,
                                double distance, const QLineF &intersectionLine, QPointF *placement,
                                Side *horizontalAlignedSide);
    static double calcAngle(const QLineF &line);
    static double calcDistancePointToLine(const QPointF &point, const QLineF &line);
    static QPointF calcProjection(const QLineF &line, const QPointF &point);
    static QPointF calcPrimaryAxisDirection(const QLineF &line);
    static QPointF calcSecondaryAxisDirection(const QLineF &line);
    static void adjustPosAndRect(QPointF *pos, QRectF *rect, const QPointF &topLeftDelta,
                                 const QPointF &bottomRightDelta, const QPointF &relativeAlignment);
    static QSizeF ensureMinimumRasterSize(const QSizeF &size, double rasterWidth,
                                          double rasterHeight);
};

} // namespace qmt
