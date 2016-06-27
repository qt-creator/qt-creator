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

#include "shapepaintvisitor.h"

#include "shapes.h"

namespace qmt {

ShapePaintVisitor::ShapePaintVisitor(QPainter *painter, const QPointF &scaledOrigin, const QSizeF &originalSize,
                                     const QSizeF &baseSize, const QSizeF &size)
    : m_painter(painter),
      m_scaledOrigin(scaledOrigin),
      m_originalSize(originalSize),
      m_baseSize(baseSize),
      m_size(size)
{
}

void ShapePaintVisitor::visitLine(const LineShape *shapeLine)
{
    QPointF p1 = shapeLine->pos1().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
    QPointF p2 = shapeLine->pos2().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
    m_painter->drawLine(p1, p2);
}

void ShapePaintVisitor::visitRect(const RectShape *shapeRect)
{
    m_painter->drawRect(QRectF(shapeRect->pos().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size),
                              shapeRect->size().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size)));
}

void ShapePaintVisitor::visitRoundedRect(const RoundedRectShape *shapeRoundedRect)
{
    qreal radiusX = shapeRoundedRect->radius().mapScaledTo(0, m_originalSize.width(), m_baseSize.width(), m_size.width());
    qreal radiusY = shapeRoundedRect->radius().mapScaledTo(0, m_originalSize.height(), m_baseSize.height(), m_size.height());
    m_painter->drawRoundedRect(QRectF(shapeRoundedRect->pos().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size),
                                     shapeRoundedRect->size().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size)),
                              radiusX, radiusY);
}

void ShapePaintVisitor::visitCircle(const CircleShape *shapeCircle)
{
    m_painter->drawEllipse(shapeCircle->center().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size),
                          shapeCircle->radius().mapScaledTo(m_scaledOrigin.x(), m_originalSize.width(), m_baseSize.width(), m_size.width()),
                          shapeCircle->radius().mapScaledTo(m_scaledOrigin.y(), m_originalSize.height(), m_baseSize.height(), m_size.height()));
}

void ShapePaintVisitor::visitEllipse(const EllipseShape *shapeEllipse)
{
    QSizeF radius = shapeEllipse->radius().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
    m_painter->drawEllipse(shapeEllipse->center().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size),
                          radius.width(), radius.height());
}

void ShapePaintVisitor::visitArc(const ArcShape *shapeArc)
{
    QSizeF radius = shapeArc->radius().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
    m_painter->drawArc(QRectF(shapeArc->center().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size) - QPointF(radius.width(), radius.height()), radius * 2.0),
                      shapeArc->startAngle() * 16, shapeArc->spanAngle() * 16);
}

void ShapePaintVisitor::visitPath(const PathShape *shapePath)
{
    QPainterPath path;
    foreach (const PathShape::Element &element, shapePath->elements()) {
        switch (element.m_elementType) {
        case PathShape::TypeNone:
            // nothing to do
            break;
        case PathShape::TypeMoveto:
            path.moveTo(element.m_position.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size));
            break;
        case PathShape::TypeLineto:
            path.lineTo(element.m_position.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size));
            break;
        case PathShape::TypeArcmoveto:
        {
            QSizeF radius = element.m_size.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
            path.arcMoveTo(QRectF(element.m_position.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size) - QPointF(radius.width(), radius.height()),
                                  radius * 2.0),
                           element.m_angle1);
            break;
        }
        case PathShape::TypeArcto:
        {
            QSizeF radius = element.m_size.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
            path.arcTo(QRectF(element.m_position.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size) - QPointF(radius.width(), radius.height()),
                              radius * 2.0),
                       element.m_angle1, element.m_angle2);
            break;
        }
        case PathShape::TypeClose:
            path.closeSubpath();
            break;
        }
    }
    m_painter->drawPath(path);
}

ShapeSizeVisitor::ShapeSizeVisitor(const QPointF &scaledOrigin, const QSizeF &originalSize, const QSizeF &baseSize,
                                   const QSizeF &size)
    : m_scaledOrigin(scaledOrigin),
      m_originalSize(originalSize),
      m_baseSize(baseSize),
      m_size(size)
{
}

void ShapeSizeVisitor::visitLine(const LineShape *shapeLine)
{
    m_boundingRect |= QRectF(shapeLine->pos1().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size),
                             shapeLine->pos2().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size));
}

void ShapeSizeVisitor::visitRect(const RectShape *shapeRect)
{
    m_boundingRect |= QRectF(shapeRect->pos().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size),
                             shapeRect->size().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size));
}

void ShapeSizeVisitor::visitRoundedRect(const RoundedRectShape *shapeRoundedRect)
{
    m_boundingRect |= QRectF(shapeRoundedRect->pos().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size),
                             shapeRoundedRect->size().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size));
}

void ShapeSizeVisitor::visitCircle(const CircleShape *shapeCircle)
{
    QSizeF radius = QSizeF(shapeCircle->radius().mapScaledTo(m_scaledOrigin.x(), m_originalSize.width(), m_baseSize.width(), m_size.width()),
                           shapeCircle->radius().mapScaledTo(m_scaledOrigin.y(), m_originalSize.height(), m_baseSize.height(), m_size.height()));
    m_boundingRect |= QRectF(shapeCircle->center().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size) - QPointF(radius.width(), radius.height()), radius * 2.0);
}

void ShapeSizeVisitor::visitEllipse(const EllipseShape *shapeEllipse)
{
    QSizeF radius = shapeEllipse->radius().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
    m_boundingRect |= QRectF(shapeEllipse->center().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size) - QPointF(radius.width(), radius.height()), radius * 2.0);
}

void ShapeSizeVisitor::visitArc(const ArcShape *shapeArc)
{
    // TODO this is the max bound rect; not the minimal one
    QSizeF radius = shapeArc->radius().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
    m_boundingRect |= QRectF(shapeArc->center().mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size) - QPointF(radius.width(), radius.height()), radius * 2.0);
}

void ShapeSizeVisitor::visitPath(const PathShape *shapePath)
{
    QPainterPath path;
    foreach (const PathShape::Element &element, shapePath->elements()) {
        switch (element.m_elementType) {
        case PathShape::TypeNone:
            // nothing to do
            break;
        case PathShape::TypeMoveto:
            path.moveTo(element.m_position.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size));
            break;
        case PathShape::TypeLineto:
            path.lineTo(element.m_position.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size));
            break;
        case PathShape::TypeArcmoveto:
        {
            QSizeF radius = element.m_size.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
            path.arcMoveTo(QRectF(element.m_position.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size) - QPointF(radius.width(), radius.height()),
                                  radius * 2.0),
                           element.m_angle1);
            break;
        }
        case PathShape::TypeArcto:
        {
            QSizeF radius = element.m_size.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size);
            path.arcTo(QRectF(element.m_position.mapScaledTo(m_scaledOrigin, m_originalSize, m_baseSize, m_size) - QPointF(radius.width(), radius.height()),
                              radius * 2.0),
                       element.m_angle1, element.m_angle2);
            break;
        }
        case PathShape::TypeClose:
            path.closeSubpath();
            break;
        }
    }
    m_boundingRect |= path.boundingRect();
}

} // namespace qmt
