// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsItem>

QT_BEGIN_NAMESPACE
class QGraphicsPathItem;
class QPainterPath;
QT_END_NAMESPACE

namespace qmt {

class Style;

class ArrowItem : public QGraphicsItem
{
    class GraphicsHeadItem;
    class GraphicsShaftItem;

public:
    enum Shaft {
        ShaftSolid,
        ShaftDashed,
        ShaftDot,
        ShaftDashDot,
        ShaftDashDotDot
    };

    enum Head {
        HeadNone,
        HeadCustom,
        HeadOpen,
        HeadTriangle,
        HeadFilledTriangle,
        HeadDiamond,
        HeadFilledDiamond,
        HeadDiamondFilledTriangle,
        HeadFilledDiamondFilledTriangle
    };

    explicit ArrowItem(QGraphicsItem *parent = nullptr);
    explicit ArrowItem(const ArrowItem &rhs, QGraphicsItem *parent = nullptr);
    ~ArrowItem() override;

    void setShaft(Shaft shaft);
    void setArrowSize(double arrowSize);
    void setDiamondSize(double diamondSize);
    void setStartHead(Head head);
    void setStartHead(QGraphicsItem *startHeadItem);
    void setEndHead(Head head);
    void setEndHead(QGraphicsItem *endHeadItem);
    void setPoints(const QList<QPointF> &points);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPainterPath shape() const override;

    QPointF calcPointAtPercent(double percentage) const;
    QLineF firstLineSegment() const;
    QLineF lastLineSegment() const;
    double startHeadLength() const;
    double endHeadLength() const;

    void update(const Style *style);

private:
    void updateShaft(const Style *style);
    void deleteHead(QGraphicsItem **headItem);
    void updateHead(QGraphicsItem **headItem, Head head, const Style *style);
    void updateHeadGeometry(QGraphicsItem *headItem, const QPointF &pos,
                            const QPointF &otherPos);
    void updateGeometry();
    double calcHeadLength(QGraphicsItem *headItem) const;

    Shaft m_shaft = ShaftSolid;
    GraphicsShaftItem *m_shaftItem = nullptr;
    double m_arrowSize = 10.0;
    double m_diamondSize = 15.0;
    Head m_startHead = HeadNone;
    QGraphicsItem *m_startHeadItem = nullptr;
    Head m_endHead = HeadNone;
    QGraphicsItem *m_endHeadItem = nullptr;
    QList<QPointF> m_points;
};

} // namespace qmt
