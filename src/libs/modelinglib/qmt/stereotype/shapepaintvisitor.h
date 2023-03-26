// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "shapevisitor.h"
#include "qmt/infrastructure/qmt_global.h"

#include <QPainter>
#include <QPointF>
#include <QSizeF>
#include <QPolygonF>
#include <QPainterPath>

namespace qmt {

class QMT_EXPORT ShapePaintVisitor : public ShapeConstVisitor
{
public:
    ShapePaintVisitor(QPainter *painter, const QPointF &scaledOrigin, const QSizeF &originalSize,
                      const QSizeF &baseSize, const QSizeF &size);

    void visitLine(const LineShape *shapeLine) override;
    void visitRect(const RectShape *shapeRect) override;
    void visitRoundedRect(const RoundedRectShape *shapeRoundedRect) override;
    void visitCircle(const CircleShape *shapeCircle) override;
    void visitEllipse(const EllipseShape *shapeEllipse) override;
    void visitDiamond(const DiamondShape *shapeDiamond) override;
    void visitTriangle(const TriangleShape *shapeTriangle) override;
    void visitArc(const ArcShape *shapeArc) override;
    void visitPath(const PathShape *shapePath) override;

private:
    QPainter *m_painter;
    QPointF m_scaledOrigin;
    QSizeF m_originalSize;
    QSizeF m_baseSize;
    QSizeF m_size;
};

class QMT_EXPORT ShapeSizeVisitor : public ShapeConstVisitor
{
public:
    ShapeSizeVisitor(const QPointF &scaledOrigin, const QSizeF &originalSize,
                     const QSizeF &baseSize, const QSizeF &size);

    QRectF boundingRect() const { return m_boundingRect; }

    void visitLine(const LineShape *shapeLine) override;
    void visitRect(const RectShape *shapeRect) override;
    void visitRoundedRect(const RoundedRectShape *shapeRoundedRect) override;
    void visitCircle(const CircleShape *shapeCircle) override;
    void visitEllipse(const EllipseShape *shapeEllipse) override;
    void visitDiamond(const DiamondShape *shapeDiamond) override;
    void visitTriangle(const TriangleShape *shapeTriangle) override;
    void visitArc(const ArcShape *shapeArc) override;
    void visitPath(const PathShape *shapePath) override;

private:
    QPointF m_scaledOrigin;
    QSizeF m_originalSize;
    QSizeF m_baseSize;
    QSizeF m_size;
    QRectF m_boundingRect;
};

class QMT_EXPORT ShapePolygonVisitor : public ShapeConstVisitor
{
public:
    ShapePolygonVisitor(const QPointF &scaledOrigin, const QSizeF &originalSize,
                        const QSizeF &baseSize, const QSizeF &size);

    QPainterPath path() const { return m_path; }
    QList<QPolygonF> toPolygons() const;

    void visitLine(const LineShape *shapeLine) override;
    void visitRect(const RectShape *shapeRect) override;
    void visitRoundedRect(const RoundedRectShape *shapeRoundedRect) override;
    void visitCircle(const CircleShape *shapeCircle) override;
    void visitEllipse(const EllipseShape *shapeEllipse) override;
    void visitDiamond(const DiamondShape *shapeDiamond) override;
    void visitTriangle(const TriangleShape *shapeTriangle) override;
    void visitArc(const ArcShape *shapeArc) override;
    void visitPath(const PathShape *shapePath) override;

private:
    QPointF m_scaledOrigin;
    QSizeF m_originalSize;
    QSizeF m_baseSize;
    QSizeF m_size;
    QPainterPath m_path;
};

} // namespace qmt
