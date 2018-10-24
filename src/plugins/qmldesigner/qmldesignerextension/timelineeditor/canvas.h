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

#pragma once

#include "canvasstyledialog.h"
#include <qglobal.h>

QT_FORWARD_DECLARE_CLASS(QBrush);
QT_FORWARD_DECLARE_CLASS(QColor);
QT_FORWARD_DECLARE_CLASS(QPainter);
QT_FORWARD_DECLARE_CLASS(QPointF);
QT_FORWARD_DECLARE_CLASS(QPoint);
QT_FORWARD_DECLARE_CLASS(QSize);

namespace QmlDesigner {

class EasingCurve;

class Canvas
{
public:
    Canvas(int width,
           int height,
           int marginX,
           int marginY,
           int cellCountX,
           int cellCountY,
           int offsetX,
           int offsetY);

public:
    CanvasStyle canvasStyle() const;

    double scale() const;

    QRectF gridRect() const;

    QPointF normalize(const QPointF &point) const;

    QPointF mapTo(const QPointF &point) const;

    EasingCurve mapTo(const EasingCurve &curve) const;

    QPointF mapFrom(const QPointF &point) const;

    EasingCurve mapFrom(const EasingCurve &curve) const;

    QPointF clamp(const QPointF &point) const;

    void setCanvasStyle(const CanvasStyle &style);

    void setScale(double scale);

    void resize(const QSize &size);

    void paintGrid(QPainter *painter, const QBrush &background);

    void paintCurve(QPainter *painter, const EasingCurve &curve, const QColor &color);

    void paintControlPoints(QPainter *painter, const EasingCurve &curve);

    void paintProgress(QPainter *painter, const EasingCurve &curve, double progress);

private:
    void paintLine(QPainter *painter, const QPoint &p1, const QPoint &p2);

    void paintPoint(QPainter *painter, const QPointF &point, bool smooth, bool active = false);

private:
    int m_width;
    int m_height;

    int m_marginX;
    int m_marginY;

    int m_cellCountX;
    int m_cellCountY;

    int m_offsetX;
    int m_offsetY;

    double m_scale;

    CanvasStyle m_style;
};

} // namespace QmlDesigner
