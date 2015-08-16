/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "geometryutilities.h"

#include "qmt/infrastructure/qmtassert.h"

#include <QPolygonF>
#include <QLineF>
#include <QPointF>
#include <QPair>
#include <QList>
#include <QVector2D>
#include <qmath.h>
#include <qdebug.h>
#include <limits>


template <typename T>
inline int sgn(T val)
{
    return (T(0) < val) - (val < T(0));
}


namespace qmt {

GeometryUtilities::GeometryUtilities()
{
}

QLineF GeometryUtilities::stretch(const QLineF &line, double p1_extension, double p2_extension)
{
    QLineF direction = line.unitVector();
    QPointF stretched_p1 = line.p1() - (direction.p2() - direction.p1()) * p1_extension;
    QPointF stretched_p2 = line.p2() + (direction.p2() - direction.p1()) * p2_extension;
    return QLineF(stretched_p1, stretched_p2);
}

bool GeometryUtilities::intersect(const QPolygonF &polygon, const QLineF &line, QPointF *intersection_point, QLineF *intersection_line)
{
    for (int i = 0; i <= polygon.size() - 2; ++i) {
        QLineF polygon_line(polygon.at(i), polygon.at(i+1));
        QLineF::IntersectType intersection_type = polygon_line.intersect(line, intersection_point);
        if (intersection_type == QLineF::BoundedIntersection) {
            if (intersection_line) {
                *intersection_line = polygon_line;
            }
            return true;
        }
    }
    return false;
}

namespace {

struct Candidate {
    Candidate() : third(GeometryUtilities::SIDE_UNSPECIFIED) { }

    Candidate(const QVector2D &f, const QPointF &s, GeometryUtilities::Side t) : first(f), second(s), third(t) { }

    QVector2D first;
    QPointF second;
    GeometryUtilities::Side third;
};

}

bool GeometryUtilities::placeRectAtLine(const QRectF &rect, const QLineF &line, double line_offset, double distance, const QLineF &intersection_line, QPointF *placement, Side *horizontal_aligned_side)
{
    QMT_CHECK(placement);

    QList<Candidate> candidates;
    candidates << Candidate(QVector2D(rect.topRight() - rect.topLeft()), QPointF(0.0, 0.0), SIDE_TOP)
               << Candidate(QVector2D(rect.topLeft() - rect.topRight()), rect.topRight() - rect.topLeft(), SIDE_TOP)
               << Candidate(QVector2D(rect.bottomLeft() - rect.topLeft()), QPointF(0.0, 0.0), SIDE_LEFT)
               << Candidate(QVector2D(rect.topLeft() - rect.bottomLeft()), rect.bottomLeft() - rect.topLeft(), SIDE_LEFT)
               << Candidate(QVector2D(rect.bottomRight() - rect.bottomLeft()), rect.bottomLeft() - rect.topLeft(), SIDE_BOTTOM)
               << Candidate(QVector2D(rect.bottomLeft() - rect.bottomRight()), rect.bottomRight() - rect.topLeft(), SIDE_BOTTOM)
               << Candidate(QVector2D(rect.bottomRight() - rect.topRight()), rect.topRight() - rect.topLeft(), SIDE_RIGHT)
               << Candidate(QVector2D(rect.topRight() - rect.bottomRight()), rect.bottomRight() - rect.topLeft(), SIDE_RIGHT);

    QVector<QVector2D> rect_edge_vectors;
    rect_edge_vectors << QVector2D(rect.topLeft() - rect.topLeft())
                      << QVector2D(rect.topRight() - rect.topLeft())
                      << QVector2D(rect.bottomLeft() - rect.topLeft())
                      << QVector2D(rect.bottomRight() -rect.topLeft());

    QVector2D direction_vector(line.p2() - line.p1());
    direction_vector.normalize();

    QVector2D side_vector(direction_vector.y(), -direction_vector.x());

    QVector2D intersection_vector(intersection_line.p2() - intersection_line.p1());
    intersection_vector.normalize();

    QVector2D outside_vector = QVector2D(intersection_vector.y(), -intersection_vector.x());
    double p = QVector2D::dotProduct(direction_vector, outside_vector);
    if (p < 0.0) {
        outside_vector = outside_vector * -1.0;
    }

    double smallest_a = -1.0;
    QPointF rect_translation;
    Side side = SIDE_UNSPECIFIED;
    int best_sign = 0;

    foreach (const Candidate &candidate, candidates) {
        // solve equation a * direction_vector + candidate.first = b * intersection_vector to find smallest a
        double r = direction_vector.x() * intersection_vector.y() - direction_vector.y() * intersection_vector.x();
        if (r <= -1e-5 || r >= 1e-5) {
            double a = (candidate.first.y() * intersection_vector.x() - candidate.first.x() * intersection_vector.y()) / r;
            if (a >= 0.0 && (smallest_a < 0.0 || a < smallest_a)) {
                // verify that all rectangle edges lay outside of shape (by checking for positiv projection to intersection)
                bool ok = true;
                int sign = 0;
                QVector2D rect_origin_vector = direction_vector * a - QVector2D(candidate.second);
                foreach (const QVector2D &rect_edge_vector, rect_edge_vectors) {
                    QVector2D edge_vector = rect_origin_vector + rect_edge_vector;
                    double aa = QVector2D::dotProduct(outside_vector, edge_vector);
                    if (aa < 0.0) {
                        ok = false;
                        break;
                    }
                    int s = sgn(QVector2D::dotProduct(side_vector, edge_vector));
                    if (s) {
                        if (sign) {
                            if (s != sign) {
                                ok = false;
                                break;
                            }
                        } else {
                            sign = s;
                        }
                    }
                }
                if (ok) {
                    smallest_a = a;
                    rect_translation = candidate.second;
                    side = candidate.third;
                    best_sign = sign;
                }
            }
        }
    }
    if (horizontal_aligned_side) {
        // convert side into a horizontal side depending on placement relative to direction vector
        switch (side) {
        case SIDE_TOP:
            side = best_sign == -1 ? SIDE_RIGHT : SIDE_LEFT;
            break;
        case SIDE_BOTTOM:
            side = best_sign == -1 ? SIDE_LEFT : SIDE_RIGHT;
            break;
        default:
            break;
        }
        *horizontal_aligned_side = side;
    }
    if (smallest_a < 0.0) {
        return false;
    }
    *placement = line.p1() + (direction_vector * (smallest_a + line_offset)).toPointF() + (side_vector * (best_sign * distance)).toPointF() - rect_translation;
    return true;
}

double GeometryUtilities::calcAngle(const QLineF &line)
{
    QVector2D direction_vector(line.p2() - line.p1());
    direction_vector.normalize();
    double angle = qAcos(direction_vector.x()) * 180.0 / 3.1415926535;
    if (direction_vector.y() > 0.0) {
        angle = -angle;
    }
    return angle;
}

namespace {

// scalar product
static qreal operator&(const QVector2D &lhs, const QVector2D &rhs) {
    return lhs.x() * rhs.x() + lhs.y() * rhs.y();
}

}

double GeometryUtilities::calcDistancePointToLine(const QPointF &point, const QLineF &line)
{
    QVector2D p(point);
    QVector2D a(line.p1());
    QVector2D direction_vector(line.p2() - line.p1());
    qreal r = -((a - p) & direction_vector) / direction_vector.lengthSquared();
    if (r < 0.0 || r > 1.0) {
        return std::numeric_limits<float>::quiet_NaN();
    }
    qreal d = (a + r * direction_vector - p).length();
    return d;
}

QPointF GeometryUtilities::calcProjection(const QLineF &line, const QPointF &point)
{
    QVector2D p(point);
    QVector2D a(line.p1());
    QVector2D direction_vector(line.p2() - line.p1());
    qreal r = -((a - p) & direction_vector) / direction_vector.lengthSquared();
    return (a + r * direction_vector).toPointF();
}

QPointF GeometryUtilities::calcPrimaryAxisDirection(const QLineF &line)
{
    qreal xAbs = qAbs(line.dx());
    qreal yAbs = qAbs(line.dy());
    if (yAbs > xAbs) {
        if (line.dy() >= 0.0) {
            return QPointF(0.0, 1.0);
        } else {
            return QPointF(0.0, -1.0);
        }
    } else {
        if (line.dx() >= 0.0) {
            return QPointF(1.0, 0.0);
        } else {
            return QPointF(-1.0, 0.0);
        }
    }
}

QPointF GeometryUtilities::calcSecondaryAxisDirection(const QLineF &line)
{
    qreal xAbs = qAbs(line.dx());
    qreal yAbs = qAbs(line.dy());
    if (yAbs > xAbs) {
        if (line.dx() >= 0.0) {
            return QPointF(1.0, 0.0);
        } else {
            return QPointF(-1.0, 0.0);
        }
    } else {
        if (line.dy() >= 0.0) {
            return QPointF(0.0, 1.0);
        } else {
            return QPointF(0.0, -1.0);
        }
    }
}

void GeometryUtilities::adjustPosAndRect(QPointF *pos, QRectF *rect, const QPointF &top_left_delta, const QPointF &bottom_right_delta, const QPointF &relative_alignment)
{
    *pos += QPointF(top_left_delta.x() * (1.0 - relative_alignment.x()) + bottom_right_delta.x() * relative_alignment.x(),
                    top_left_delta.y() * (1.0 - relative_alignment.y()) + bottom_right_delta.y() * relative_alignment.y());
    rect->adjust(top_left_delta.x() * relative_alignment.x() - bottom_right_delta.x() * relative_alignment.x(),
                 top_left_delta.y() * relative_alignment.y() - bottom_right_delta.y() * relative_alignment.y(),
                 bottom_right_delta.x() * (1.0 - relative_alignment.x()) - top_left_delta.x() * (1.0 - relative_alignment.x()),
                 bottom_right_delta.y() * (1.0 - relative_alignment.y()) - top_left_delta.y() * (1.0 - relative_alignment.y()));
}

QSizeF GeometryUtilities::ensureMinimumRasterSize(const QSizeF &size, double raster_width, double raster_height)
{
    double width = int(size.width() / raster_width + 0.99999) * raster_width;
    double height = int(size.height() / raster_height + 0.99999) * raster_height;
    return QSizeF(width, height);
}

}

