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

#pragma once

#include <QGraphicsItem>

QT_BEGIN_NAMESPACE
class QGraphicsPathItem;
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
        ShaftDashed
    };

    enum Head {
        HeadNone,
        HeadOpen,
        HeadTriangle,
        HeadFilledTriangle,
        HeadDiamond,
        HeadFilledDiamond,
        HeadDiamondFilledTriangle,
        HeadFilledDiamondFilledTriangle
    };

    explicit ArrowItem(QGraphicsItem *parent = 0);
    explicit ArrowItem(const ArrowItem &rhs, QGraphicsItem *parent = 0);
    ~ArrowItem() override;

    void setShaft(Shaft shaft);
    void setArrowSize(double arrowSize);
    void setDiamondSize(double diamondSize);
    void setStartHead(Head head);
    void setEndHead(Head head);
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
    void updateHead(GraphicsHeadItem **headItem, Head head, const Style *style);
    void updateHeadGeometry(GraphicsHeadItem **headItem, const QPointF &pos,
                            const QPointF &otherPos);
    void updateGeometry();

    Shaft m_shaft = ShaftSolid;
    GraphicsShaftItem *m_shaftItem = nullptr;
    double m_arrowSize = 10.0;
    double m_diamondSize = 15.0;
    Head m_startHead = HeadNone;
    GraphicsHeadItem *m_startHeadItem = nullptr;
    Head m_endHead = HeadNone;
    GraphicsHeadItem *m_endHeadItem = nullptr;
    QList<QPointF> m_points;
};

} // namespace qmt
