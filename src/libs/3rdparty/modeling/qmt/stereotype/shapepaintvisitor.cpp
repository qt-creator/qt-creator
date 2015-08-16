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

#include "shapepaintvisitor.h"

#include "shapes.h"

namespace qmt {

ShapePaintVisitor::ShapePaintVisitor(QPainter *painter, const QPointF &scaled_origin, const QSizeF &original_size, const QSizeF &base_size, const QSizeF &size)
    : _painter(painter),
      _scaled_origin(scaled_origin),
      _original_size(original_size),
      _base_size(base_size),
      _size(size)
{
}

void ShapePaintVisitor::visitLine(const LineShape *shape_line)
{
    QPointF p1 = shape_line->getPos1().mapScaledTo(_scaled_origin, _original_size, _base_size, _size);
    QPointF p2 = shape_line->getPos2().mapScaledTo(_scaled_origin, _original_size, _base_size, _size);
    _painter->save();
    _painter->setRenderHint(QPainter::Antialiasing, p1.x() != p2.x() && p1.y() != p2.y());
    _painter->drawLine(p1, p2);
    _painter->restore();
}

void ShapePaintVisitor::visitRect(const RectShape *shape_rect)
{
    _painter->save();
    _painter->setRenderHint(QPainter::Antialiasing, false);
    _painter->drawRect(QRectF(shape_rect->getPos().mapScaledTo(_scaled_origin, _original_size, _base_size, _size),
                              shape_rect->getSize().mapScaledTo(_scaled_origin, _original_size, _base_size, _size)));
    _painter->restore();
}

void ShapePaintVisitor::visitRoundedRect(const RoundedRectShape *shape_rounded_rect)
{
    qreal radius_x = shape_rounded_rect->getRadius().mapScaledTo(0, _original_size.width(), _base_size.width(), _size.width());
    qreal radius_y = shape_rounded_rect->getRadius().mapScaledTo(0, _original_size.height(), _base_size.height(), _size.height());
    _painter->save();
    _painter->setRenderHint(QPainter::Antialiasing, true);
    _painter->drawRoundedRect(QRectF(shape_rounded_rect->getPos().mapScaledTo(_scaled_origin, _original_size, _base_size, _size),
                                     shape_rounded_rect->getSize().mapScaledTo(_scaled_origin, _original_size, _base_size, _size)),
                              radius_x, radius_y);
    _painter->restore();
}

void ShapePaintVisitor::visitCircle(const CircleShape *shape_circle)
{
    _painter->save();
    _painter->setRenderHint(QPainter::Antialiasing, true);
    _painter->drawEllipse(shape_circle->getCenter().mapScaledTo(_scaled_origin, _original_size, _base_size, _size),
                          shape_circle->getRadius().mapScaledTo(_scaled_origin.x(), _original_size.width(), _base_size.width(), _size.width()),
                          shape_circle->getRadius().mapScaledTo(_scaled_origin.y(), _original_size.height(), _base_size.height(), _size.height()));
    _painter->restore();
}

void ShapePaintVisitor::visitEllipse(const EllipseShape *shape_ellipse)
{
    QSizeF radius = shape_ellipse->getRadius().mapScaledTo(_scaled_origin, _original_size, _base_size, _size);
    _painter->save();
    _painter->setRenderHint(QPainter::Antialiasing, true);
    _painter->drawEllipse(shape_ellipse->getCenter().mapScaledTo(_scaled_origin, _original_size, _base_size, _size),
                          radius.width(), radius.height());
    _painter->restore();
}

void ShapePaintVisitor::visitArc(const ArcShape *shape_arc)
{
    QSizeF radius = shape_arc->getRadius().mapScaledTo(_scaled_origin, _original_size, _base_size, _size);
    _painter->save();
    _painter->setRenderHint(QPainter::Antialiasing, true);
    _painter->drawArc(QRectF(shape_arc->getCenter().mapScaledTo(_scaled_origin, _original_size, _base_size, _size) - QPointF(radius.width(), radius.height()), radius * 2.0),
                      shape_arc->getStartAngle() * 16, shape_arc->getSpanAngle() * 16);
    _painter->restore();
}

void ShapePaintVisitor::visitPath(const PathShape *shape_path)
{
    _painter->save();
    _painter->setRenderHint(QPainter::Antialiasing, true);
    QPainterPath path;
    foreach (const PathShape::Element &element, shape_path->getElements()) {
        switch (element._element_type) {
        case PathShape::TYPE_NONE:
            // nothing to do
            break;
        case PathShape::TYPE_MOVETO:
            path.moveTo(element._position.mapScaledTo(_scaled_origin, _original_size, _base_size, _size));
            break;
        case PathShape::TYPE_LINETO:
            path.lineTo(element._position.mapScaledTo(_scaled_origin, _original_size, _base_size, _size));
            break;
        case PathShape::TYPE_ARCMOVETO:
        {
            QSizeF radius = element._size.mapScaledTo(_scaled_origin, _original_size, _base_size, _size);
            path.arcMoveTo(QRectF(element._position.mapScaledTo(_scaled_origin, _original_size, _base_size, _size) - QPointF(radius.width(), radius.height()),
                                  radius * 2.0),
                           element._angle1);
            break;
        }
        case PathShape::TYPE_ARCTO:
        {
            QSizeF radius = element._size.mapScaledTo(_scaled_origin, _original_size, _base_size, _size);
            path.arcTo(QRectF(element._position.mapScaledTo(_scaled_origin, _original_size, _base_size, _size) - QPointF(radius.width(), radius.height()),
                              radius * 2.0),
                       element._angle1, element._angle2);
            break;
        }
        case PathShape::TYPE_CLOSE:
            path.closeSubpath();
            break;
        }
    }
    _painter->drawPath(path);
    _painter->restore();
}


ShapeSizeVisitor::ShapeSizeVisitor(const QPointF &scaled_origin, const QSizeF &original_size, const QSizeF &base_size, const QSizeF &size)
    : _scaled_origin(scaled_origin),
      _original_size(original_size),
      _base_size(base_size),
      _size(size)
{
}

void ShapeSizeVisitor::visitLine(const LineShape *shape_line)
{
    _bounding_rect |= QRectF(shape_line->getPos1().mapScaledTo(_scaled_origin, _original_size, _base_size, _size),
                             shape_line->getPos2().mapScaledTo(_scaled_origin, _original_size, _base_size, _size));
}

void ShapeSizeVisitor::visitRect(const RectShape *shape_rect)
{
    _bounding_rect |= QRectF(shape_rect->getPos().mapScaledTo(_scaled_origin, _original_size, _base_size, _size),
                             shape_rect->getSize().mapScaledTo(_scaled_origin, _original_size, _base_size, _size));
}

void ShapeSizeVisitor::visitRoundedRect(const RoundedRectShape *shape_rounded_rect)
{
    _bounding_rect |= QRectF(shape_rounded_rect->getPos().mapScaledTo(_scaled_origin, _original_size, _base_size, _size),
                             shape_rounded_rect->getSize().mapScaledTo(_scaled_origin, _original_size, _base_size, _size));
}

void ShapeSizeVisitor::visitCircle(const CircleShape *shape_circle)
{
    QSizeF radius = QSizeF(shape_circle->getRadius().mapScaledTo(_scaled_origin.x(), _original_size.width(), _base_size.width(), _size.width()),
                           shape_circle->getRadius().mapScaledTo(_scaled_origin.y(), _original_size.height(), _base_size.height(), _size.height()));
    _bounding_rect |= QRectF(shape_circle->getCenter().mapScaledTo(_scaled_origin, _original_size, _base_size, _size) - QPointF(radius.width(), radius.height()), radius * 2.0);
}

void ShapeSizeVisitor::visitEllipse(const EllipseShape *shape_ellipse)
{
    QSizeF radius = shape_ellipse->getRadius().mapScaledTo(_scaled_origin, _original_size, _base_size, _size);
    _bounding_rect |= QRectF(shape_ellipse->getCenter().mapScaledTo(_scaled_origin, _original_size, _base_size, _size) - QPointF(radius.width(), radius.height()), radius * 2.0);
}

void ShapeSizeVisitor::visitArc(const ArcShape *shape_arc)
{
    // TODO this is the max bound rect; not the minimal one
    QSizeF radius = shape_arc->getRadius().mapScaledTo(_scaled_origin, _original_size, _base_size, _size);
    _bounding_rect |= QRectF(shape_arc->getCenter().mapScaledTo(_scaled_origin, _original_size, _base_size, _size) - QPointF(radius.width(), radius.height()), radius * 2.0);
}

void ShapeSizeVisitor::visitPath(const PathShape *shape_path)
{
    QPainterPath path;
    foreach (const PathShape::Element &element, shape_path->getElements()) {
        switch (element._element_type) {
        case PathShape::TYPE_NONE:
            // nothing to do
            break;
        case PathShape::TYPE_MOVETO:
            path.moveTo(element._position.mapScaledTo(_scaled_origin, _original_size, _base_size, _size));
            break;
        case PathShape::TYPE_LINETO:
            path.lineTo(element._position.mapScaledTo(_scaled_origin, _original_size, _base_size, _size));
            break;
        case PathShape::TYPE_ARCMOVETO:
        {
            QSizeF radius = element._size.mapScaledTo(_scaled_origin, _original_size, _base_size, _size);
            path.arcMoveTo(QRectF(element._position.mapScaledTo(_scaled_origin, _original_size, _base_size, _size) - QPointF(radius.width(), radius.height()),
                                  radius * 2.0),
                           element._angle1);
            break;
        }
        case PathShape::TYPE_ARCTO:
        {
            QSizeF radius = element._size.mapScaledTo(_scaled_origin, _original_size, _base_size, _size);
            path.arcTo(QRectF(element._position.mapScaledTo(_scaled_origin, _original_size, _base_size, _size) - QPointF(radius.width(), radius.height()),
                              radius * 2.0),
                       element._angle1, element._angle2);
            break;
        }
        case PathShape::TYPE_CLOSE:
            path.closeSubpath();
            break;
        }
    }
    _bounding_rect |= path.boundingRect();
}


} // namespace qmt
