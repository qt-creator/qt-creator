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
    : m_painter(painter),
      m_scaledOrigin(scaled_origin),
      m_originalSize(original_size),
      m_baseSize(base_size),
      m_size(size)
{
}

void ShapePaintVisitor::visitLine(const LineShape *shape_line)
{
    QPointF p1 = shape_line->getPos1().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
    QPointF p2 = shape_line->getPos2().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
    m_painter->save();
    m_painter->setRenderHint(QPainter::Antialiasing, p1.x() != p2.x() && p1.y() != p2.y());
    m_painter->drawLine(p1, p2);
    m_painter->restore();
}

void ShapePaintVisitor::visitRect(const RectShape *shape_rect)
{
    m_painter->save();
    m_painter->setRenderHint(QPainter::Antialiasing, false);
    m_painter->drawRect(QRectF(shape_rect->getPos().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size),
                              shape_rect->getSize().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size)));
    m_painter->restore();
}

void ShapePaintVisitor::visitRoundedRect(const RoundedRectShape *shape_rounded_rect)
{
    qreal radius_x = shape_rounded_rect->getRadius().mapScaledTo(0, m_originalSize.width(), m_baseSize.width(), m_size.width());
    qreal radius_y = shape_rounded_rect->getRadius().mapScaledTo(0, m_originalSize.height(), m_baseSize.height(), m_size.height());
    m_painter->save();
    m_painter->setRenderHint(QPainter::Antialiasing, true);
    m_painter->drawRoundedRect(QRectF(shape_rounded_rect->getPos().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size),
                                     shape_rounded_rect->getSize().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size)),
                              radius_x, radius_y);
    m_painter->restore();
}

void ShapePaintVisitor::visitCircle(const CircleShape *shape_circle)
{
    m_painter->save();
    m_painter->setRenderHint(QPainter::Antialiasing, true);
    m_painter->drawEllipse(shape_circle->getCenter().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size),
                          shape_circle->getRadius().mapScaledTo(m_scaledOrigin.x(), m_originalSize.width(), m_baseSize.width(), m_size.width()),
                          shape_circle->getRadius().mapScaledTo(m_scaledOrigin.y(), m_originalSize.height(), m_baseSize.height(), m_size.height()));
    m_painter->restore();
}

void ShapePaintVisitor::visitEllipse(const EllipseShape *shape_ellipse)
{
    QSizeF radius = shape_ellipse->getRadius().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
    m_painter->save();
    m_painter->setRenderHint(QPainter::Antialiasing, true);
    m_painter->drawEllipse(shape_ellipse->getCenter().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size),
                          radius.width(), radius.height());
    m_painter->restore();
}

void ShapePaintVisitor::visitArc(const ArcShape *shape_arc)
{
    QSizeF radius = shape_arc->getRadius().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
    m_painter->save();
    m_painter->setRenderHint(QPainter::Antialiasing, true);
    m_painter->drawArc(QRectF(shape_arc->getCenter().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size) - QPointF(radius.width(), radius.height()), radius * 2.0),
                      shape_arc->getStartAngle() * 16, shape_arc->getSpanAngle() * 16);
    m_painter->restore();
}

void ShapePaintVisitor::visitPath(const PathShape *shape_path)
{
    m_painter->save();
    m_painter->setRenderHint(QPainter::Antialiasing, true);
    QPainterPath path;
    foreach (const PathShape::Element &element, shape_path->getElements()) {
        switch (element.m_elementType) {
        case PathShape::TYPE_NONE:
            // nothing to do
            break;
        case PathShape::TYPE_MOVETO:
            path.moveTo(element.m_position.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size));
            break;
        case PathShape::TYPE_LINETO:
            path.lineTo(element.m_position.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size));
            break;
        case PathShape::TYPE_ARCMOVETO:
        {
            QSizeF radius = element.m_size.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
            path.arcMoveTo(QRectF(element.m_position.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size) - QPointF(radius.width(), radius.height()),
                                  radius * 2.0),
                           element.m_angle1);
            break;
        }
        case PathShape::TYPE_ARCTO:
        {
            QSizeF radius = element.m_size.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
            path.arcTo(QRectF(element.m_position.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size) - QPointF(radius.width(), radius.height()),
                              radius * 2.0),
                       element.m_angle1, element.m_angle2);
            break;
        }
        case PathShape::TYPE_CLOSE:
            path.closeSubpath();
            break;
        }
    }
    m_painter->drawPath(path);
    m_painter->restore();
}


ShapeSizeVisitor::ShapeSizeVisitor(const QPointF &scaled_origin, const QSizeF &original_size, const QSizeF &base_size, const QSizeF &size)
    : m_scaledOrigin(scaled_origin),
      m_originalSize(original_size),
      m_baseSize(base_size),
      m_size(size)
{
}

void ShapeSizeVisitor::visitLine(const LineShape *shape_line)
{
    m_boundingRect |= QRectF(shape_line->getPos1().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size),
                             shape_line->getPos2().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size));
}

void ShapeSizeVisitor::visitRect(const RectShape *shape_rect)
{
    m_boundingRect |= QRectF(shape_rect->getPos().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size),
                             shape_rect->getSize().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size));
}

void ShapeSizeVisitor::visitRoundedRect(const RoundedRectShape *shape_rounded_rect)
{
    m_boundingRect |= QRectF(shape_rounded_rect->getPos().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size),
                             shape_rounded_rect->getSize().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size));
}

void ShapeSizeVisitor::visitCircle(const CircleShape *shape_circle)
{
    QSizeF radius = QSizeF(shape_circle->getRadius().mapScaledTo(m_scaledOrigin.x(), m_originalSize.width(), m_baseSize.width(), m_size.width()),
                           shape_circle->getRadius().mapScaledTo(m_scaledOrigin.y(), m_originalSize.height(), m_baseSize.height(), m_size.height()));
    m_boundingRect |= QRectF(shape_circle->getCenter().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size) - QPointF(radius.width(), radius.height()), radius * 2.0);
}

void ShapeSizeVisitor::visitEllipse(const EllipseShape *shape_ellipse)
{
    QSizeF radius = shape_ellipse->getRadius().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
    m_boundingRect |= QRectF(shape_ellipse->getCenter().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size) - QPointF(radius.width(), radius.height()), radius * 2.0);
}

void ShapeSizeVisitor::visitArc(const ArcShape *shape_arc)
{
    // TODO this is the max bound rect; not the minimal one
    QSizeF radius = shape_arc->getRadius().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
    m_boundingRect |= QRectF(shape_arc->getCenter().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size) - QPointF(radius.width(), radius.height()), radius * 2.0);
}

void ShapeSizeVisitor::visitPath(const PathShape *shape_path)
{
    QPainterPath path;
    foreach (const PathShape::Element &element, shape_path->getElements()) {
        switch (element.m_elementType) {
        case PathShape::TYPE_NONE:
            // nothing to do
            break;
        case PathShape::TYPE_MOVETO:
            path.moveTo(element.m_position.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size));
            break;
        case PathShape::TYPE_LINETO:
            path.lineTo(element.m_position.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size));
            break;
        case PathShape::TYPE_ARCMOVETO:
        {
            QSizeF radius = element.m_size.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
            path.arcMoveTo(QRectF(element.m_position.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size) - QPointF(radius.width(), radius.height()),
                                  radius * 2.0),
                           element.m_angle1);
            break;
        }
        case PathShape::TYPE_ARCTO:
        {
            QSizeF radius = element.m_size.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
            path.arcTo(QRectF(element.m_position.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size) - QPointF(radius.width(), radius.height()),
                              radius * 2.0),
                       element.m_angle1, element.m_angle2);
            break;
        }
        case PathShape::TYPE_CLOSE:
            path.closeSubpath();
            break;
        }
    }
    m_boundingRect |= path.boundingRect();
}


} // namespace qmt
