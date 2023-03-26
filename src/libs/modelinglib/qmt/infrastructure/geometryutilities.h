// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>
#include <QList>

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
                          QPointF *intersectionPoint = nullptr, QLineF *intersectionLine = nullptr,
                          int nearestPoint = 1);
    static bool intersect(const QList<QPolygonF> &polygons, const QLineF &line,
                          int *intersectionPolygon, QPointF *intersectionPoint = nullptr, QLineF *intersectionLine = nullptr,
                          int nearestPoint = 1);
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
