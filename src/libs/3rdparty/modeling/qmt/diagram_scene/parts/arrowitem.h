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

#ifndef QMT_GRAPHICSARROWITEM_H
#define QMT_GRAPHICSARROWITEM_H

#include <QGraphicsItem>

QT_BEGIN_NAMESPACE
class QGraphicsPathItem;
QT_END_NAMESPACE


namespace qmt {

class Style;

class ArrowItem :
        public QGraphicsItem
{
    class GraphicsPathItem;
    class GraphicsHeadItem;
    class GraphicsShaftItem;

public:

    enum Shaft {
        SHAFT_SOLID,
        SHAFT_DASHED
    };

    enum Head {
        HEAD_NONE,
        HEAD_OPEN,
        HEAD_TRIANGLE,
        HEAD_FILLED_TRIANGLE,
        HEAD_DIAMOND,
        HEAD_FILLED_DIAMOND,
        HEAD_DIAMOND_FILLED_TRIANGLE,
        HEAD_FILLED_DIAMOND_FILLED_TRIANGLE
    };

public:

    ArrowItem(QGraphicsItem *parent = 0);

    ArrowItem(const ArrowItem &rhs, QGraphicsItem *parent = 0);

    ~ArrowItem();

public:

    void setShaft(Shaft shaft);

    void setArrowSize(double arrow_size);

    void setDiamondSize(double diamond_size);

    void setStartHead(Head head);

    void setEndHead(Head head);

    void setPoints(const QList<QPointF> &points);

public:

    QRectF boundingRect() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    QPainterPath shape() const;

public:

    QPointF calcPointAtPercent(double percentage) const;

    QLineF getFirstLineSegment() const;

    QLineF getLastLineSegment() const;

    double getStartHeadLength() const;

    double getEndHeadLength() const;

    void update(const Style *style);

private:

    void updateShaft(const Style *style);

    void updateHead(GraphicsHeadItem **head_item, Head head, const Style *style);

    void updateHeadGeometry(GraphicsHeadItem **head_item, const QPointF &pos, const QPointF &other_pos);

    void updateGeometry();

private:

    Shaft m_shaft;

    GraphicsShaftItem *m_shaftItem;

    double m_arrowSize;

    double m_diamondSize;

    Head m_startHead;

    GraphicsHeadItem *m_startHeadItem;

    Head m_endHead;

    GraphicsHeadItem *m_endHeadItem;

    QList<QPointF> m_points;
};

}

#endif // QMT_GRAPHICSARROWITEM_H
