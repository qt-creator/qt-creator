/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "canvas.h"
#include "easingcurve.h"

#include <QPainter>
#include <QPointF>
#include <QSize>

namespace QmlDesigner {

Canvas::Canvas(int width,
               int height,
               int marginX,
               int marginY,
               int cellCountX,
               int cellCountY,
               int offsetX,
               int offsetY)
    : m_width(width)
    , m_height(height)
    , m_marginX(marginX)
    , m_marginY(marginY)
    , m_cellCountX(cellCountX)
    , m_cellCountY(cellCountY)
    , m_offsetX(offsetX)
    , m_offsetY(offsetY)
    , m_scale(1.0)
{}

QRectF Canvas::gridRect() const
{
    double w = static_cast<double>(m_width);
    double h = static_cast<double>(m_height);
    double mx = static_cast<double>(m_marginX);
    double my = static_cast<double>(m_marginY);
    double gw = w - 2.0 * mx;
    double gh = h - 2.0 * my;

    if (m_style.aspect != 0) {
        if (m_style.aspect < (w / h))
            gw = gh * m_style.aspect;
        else
            gh = gw / m_style.aspect;
    }

    auto rect = QRectF(mx, my, gw * m_scale, gh * m_scale);
    rect.moveCenter(QPointF(w / 2.0, h / 2.0));
    return rect;
}

void Canvas::setCanvasStyle(const CanvasStyle &style)
{
    m_style = style;
}

void Canvas::setScale(double scale)
{
    if (scale > 0.05)
        m_scale = scale;
}

void Canvas::resize(const QSize &size)
{
    m_width = size.width();
    m_height = size.height();
}

void Canvas::paintGrid(QPainter *painter, const QBrush &background)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPen pen = painter->pen();

    pen.setWidthF(m_style.thinLineWidth);
    pen.setColor(m_style.thinLineColor);
    painter->setPen(pen);

    painter->fillRect(0, 0, m_width, m_height, background);

    QRectF rect = gridRect();

    // Thin lines.
    const int lineCountX = m_cellCountX + 1;
    const double cellWidth = rect.width() / static_cast<double>(m_cellCountX);

    // Vertical
    double x = rect.left();
    for (int i = 0; i < lineCountX; ++i) {
        paintLine(painter, QPoint(x, rect.top()), QPoint(x, rect.bottom()));
        x += cellWidth;
    }

    const int lineCountY = m_cellCountY + 1;
    const double cellHeight = rect.height() / static_cast<double>(m_cellCountY);

    // Horizontal
    double y = rect.top();
    for (int i = 0; i < lineCountY; ++i) {
        paintLine(painter, QPoint(rect.left(), y), QPoint(rect.right(), y));
        y += cellHeight;
    }

    // Thick lines.
    pen.setWidthF(m_style.thickLineWidth);
    pen.setColor(m_style.thickLineColor);
    painter->setPen(pen);

    if (m_offsetX != 0) {
        const int minX = rect.left() + (cellWidth * m_offsetX);
        const int maxX = rect.right() - (cellWidth * m_offsetX);
        paintLine(painter, QPoint(minX, rect.top()), QPoint(minX, rect.bottom()));
        paintLine(painter, QPoint(maxX, rect.top()), QPoint(maxX, rect.bottom()));
    }

    if (m_offsetY != 0) {
        const int minY = rect.top() + (cellHeight * m_offsetY);
        const int maxY = rect.bottom() - (cellHeight * m_offsetY);
        paintLine(painter, QPoint(rect.left(), minY), QPoint(rect.right(), minY));
        paintLine(painter, QPoint(rect.left(), maxY), QPoint(rect.right(), maxY));
    }

    painter->restore();
}

void Canvas::paintCurve(QPainter *painter, const EasingCurve &curve, const QColor &color)
{
    EasingCurve mapped = mapTo(curve);
    painter->strokePath(mapped.path(), QPen(QBrush(color), m_style.curveWidth));
}

void Canvas::paintControlPoints(QPainter *painter, const EasingCurve &curve)
{
    QVector<QPointF> points = curve.toCubicSpline();
    int count = points.count();

    if (count <= 1)
        return;

    painter->save();

    QPen pen = painter->pen();
    pen.setWidthF(m_style.handleLineWidth);
    pen.setColor(m_style.endPointColor);

    painter->setPen(pen);
    painter->setBrush(m_style.endPointColor);

    // First and last point including handle.
    paintLine(painter, mapTo(QPointF(0.0, 0.0)).toPoint(), mapTo(points.at(0)).toPoint());
    paintPoint(painter, QPointF(0.0, 0.0), false);
    paintPoint(painter, points.at(0), false, curve.active() == 0);

    paintLine(painter, mapTo(QPointF(1.0, 1.0)).toPoint(), mapTo(points.at(count - 2)).toPoint());
    paintPoint(painter, QPointF(1.0, 1.0), false);
    paintPoint(painter, points.at(count - 2), false, curve.active() == (count - 2));

    pen.setColor(m_style.interPointColor);
    painter->setPen(pen);
    painter->setBrush(m_style.interPointColor);

    for (int i = 0; i < count - 1; ++i) {
        if (curve.isHandle(i))
            continue;

        paintLine(painter, mapTo(points.at(i)).toPoint(), mapTo(points.at(i + 1)).toPoint());

        if (i > 0)
            paintLine(painter, mapTo(points.at(i - 1)).toPoint(), mapTo(points.at(i)).toPoint());
    }

    // Paint Points.
    int active = curve.active();
    for (int i = 1; i < count - 2; ++i)
        paintPoint(painter, points.at(i), curve.isSmooth(i), active == i);

    painter->restore();
}

void Canvas::paintProgress(QPainter *painter, const EasingCurve &curve, double progress)
{
    painter->save();

    painter->setPen(Qt::green);
    painter->setBrush(QBrush(Qt::green));

    QPointF pos1(progress, curve.valueForProgress(progress));
    pos1 = mapTo(pos1);

    QRectF rect = gridRect();

    painter->drawLine(rect.left(), pos1.y(), rect.right(), pos1.y());
    painter->drawLine(pos1.x(), rect.top(), pos1.x(), rect.bottom());

    painter->restore();
}

QPointF Canvas::mapTo(const QPointF &point) const
{
    QRectF rect = gridRect();

    const double cellWidth = rect.width() / static_cast<double>(m_cellCountX);
    const double cellHeight = rect.height() / static_cast<double>(m_cellCountY);

    const double offsetX = cellWidth * m_offsetX;
    const double offsetY = cellHeight * m_offsetY;

    const int width = rect.width() - 2 * offsetX;
    const int height = rect.height() - 2 * offsetY;

    auto tmp = QPointF(point.x() * width + rect.left() + offsetX,
                       height - point.y() * height + rect.top() + offsetY);

    return tmp;
}

CanvasStyle Canvas::canvasStyle() const
{
    return m_style;
}

double Canvas::scale() const
{
    return m_scale;
}

QPointF Canvas::normalize(const QPointF &point) const
{
    QRectF rect = gridRect();
    return QPointF(point.x() / rect.width(), point.y() / rect.height());
}

EasingCurve Canvas::mapTo(const EasingCurve &curve) const
{
    QVector<QPointF> controlPoints = curve.toCubicSpline();

    for (auto &point : controlPoints)
        point = mapTo(point);

    return EasingCurve(mapTo(curve.start()), controlPoints);
}

QPointF Canvas::mapFrom(const QPointF &point) const
{
    QRectF rect = gridRect();

    const double cellWidth = rect.width() / static_cast<double>(m_cellCountX);
    const double cellHeight = rect.height() / static_cast<double>(m_cellCountY);

    const double offsetX = cellWidth * m_offsetX;
    const double offsetY = cellHeight * m_offsetY;

    const int width = rect.width() - 2 * offsetX;
    const int height = rect.height() - 2 * offsetY;

    return QPointF((point.x() - rect.left() - offsetX) / width,
                   1 - (point.y() - rect.top() - offsetY) / height);
}

EasingCurve Canvas::mapFrom(const EasingCurve &curve) const
{
    QVector<QPointF> controlPoints = curve.toCubicSpline();
    for (auto &point : controlPoints)
        point = mapFrom(point);

    EasingCurve result;
    result.fromCubicSpline(controlPoints);
    return result;
}

QPointF Canvas::clamp(const QPointF &point) const
{
    QRectF r = gridRect();
    QPointF p = point;

    if (p.x() > r.right())
        p.rx() = r.right();

    if (p.x() < r.left())
        p.rx() = r.left();

    if (p.y() < r.top())
        p.ry() = r.top();

    if (p.y() > r.bottom())
        p.ry() = r.bottom();

    return p;
}

void Canvas::paintLine(QPainter *painter, const QPoint &p1, const QPoint &p2)
{
    painter->drawLine(p1 + QPointF(0.5, 0.5), p2 + QPointF(0.5, 0.5));
}

void Canvas::paintPoint(QPainter *painter, const QPointF &point, bool smooth, bool active)
{
    const double pointSize = m_style.handleSize;
    const double activePointSize = pointSize + 2;
    if (smooth) {
        if (active) {
            painter->save();
            painter->setPen(Qt::white);
            painter->setBrush(QBrush());
            painter->drawEllipse(QRectF(mapTo(point).x() - activePointSize + 0.5,
                                        mapTo(point).y() - activePointSize + 0.5,
                                        activePointSize * 2,
                                        activePointSize * 2));
            painter->restore();
        }

        painter->drawEllipse(QRectF(mapTo(point).x() - pointSize + 0.5,
                                    mapTo(point).y() - pointSize + 0.5,
                                    pointSize * 2,
                                    pointSize * 2));

    } else {
        if (active) {
            painter->save();
            painter->setPen(Qt::white);
            painter->setBrush(QBrush());
            painter->drawRect(QRectF(mapTo(point).x() - activePointSize + 0.5,
                                     mapTo(point).y() - activePointSize + 0.5,
                                     activePointSize * 2,
                                     activePointSize * 2));
            painter->restore();
        }
        painter->drawRect(QRectF(mapTo(point).x() - pointSize + 0.5,
                                 mapTo(point).y() - pointSize + 0.5,
                                 pointSize * 2,
                                 pointSize * 2));
    }
}

} // namespace QmlDesigner
