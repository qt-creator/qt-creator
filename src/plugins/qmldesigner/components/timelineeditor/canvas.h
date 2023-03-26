// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
