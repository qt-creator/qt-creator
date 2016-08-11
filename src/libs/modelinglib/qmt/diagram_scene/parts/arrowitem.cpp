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

#include "arrowitem.h"

#include "qmt/infrastructure/qmtassert.h"
#include "qmt/style/style.h"

#include <QPainterPath>
#include <QPen>
#include <qmath.h>
#include <QVector2D>
#include <qdebug.h>

#include <QGraphicsScene>
#include <QPainter>
#include <QPen>

namespace qmt {

class ArrowItem::GraphicsHeadItem : public QGraphicsItem
{
public:
    explicit GraphicsHeadItem(QGraphicsItem *parent)
        : QGraphicsItem(parent)
    {
    }

    ~GraphicsHeadItem() override
    {
    }

    void setHead(ArrowItem::Head head)
    {
        if (m_head != head)
            m_head = head;
    }

    void setArrowSize(double arrowSize)
    {
        if (m_arrowSize != arrowSize)
            m_arrowSize = arrowSize;
    }

    void setDiamondSize(double diamondSize)
    {
        if (m_diamondSize != diamondSize)
            m_diamondSize = diamondSize;
    }

    QRectF boundingRect() const override
    {
        return childrenBoundingRect();
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override
    {
        Q_UNUSED(painter);
        Q_UNUSED(option);
        Q_UNUSED(widget);

#ifdef DEBUG_PAINT_SHAPE
        painter->setPen(QPen(Qt::blue));
        painter->drawRect(boundingRect());
        painter->setPen(QPen(Qt::red));
        painter->drawPath(shape());
        painter->setPen(QPen(Qt::green));
        if (m_arrowItem)
            painter->drawRect(mapRectFromItem(m_arrowItem, m_arrowItem->boundingRect()));
        if (m_diamondItem)
            painter->drawRect(mapRectFromItem(m_diamondItem, m_diamondItem->boundingRect()));
#endif
    }

    QPainterPath shape() const override
    {
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);
        if (m_arrowItem)
            path.addRect(mapRectFromItem(m_arrowItem, m_arrowItem->boundingRect()));
        if (m_diamondItem)
            path.addRect(mapRectFromItem(m_diamondItem, m_diamondItem->boundingRect()));
        return path;
    }

    double calcHeadLength() const
    {
        double length = 0.0;
        switch (m_head) {
        case ArrowItem::HeadNone:
            break;
        case ArrowItem::HeadOpen:
        case ArrowItem::HeadTriangle:
        case ArrowItem::HeadFilledTriangle:
            length = calcArrowLength();
            break;
        case ArrowItem::HeadDiamond:
        case ArrowItem::HeadFilledDiamond:
            length = calcDiamondLength();
            break;
        case ArrowItem::HeadDiamondFilledTriangle:
        case ArrowItem::HeadFilledDiamondFilledTriangle:
            length = calcDiamondLength() + calcArrowLength();
            break;
        }
        return length;
    }

private:
    double calcArrowLength() const
    {
        return sqrt(3.0) * 0.5 * m_arrowSize;
    }

    double calcDiamondLength() const
    {
        return sqrt(3.0) * m_diamondSize;
    }

public:
    void update(const Style *style)
    {
        bool hasArrow = false;
        bool hasDiamond = false;
        switch (m_head) {
        case ArrowItem::HeadNone:
            break;
        case ArrowItem::HeadOpen:
        case ArrowItem::HeadTriangle:
        case ArrowItem::HeadFilledTriangle:
            hasArrow = true;
            break;
        case ArrowItem::HeadDiamond:
        case ArrowItem::HeadFilledDiamond:
            hasDiamond = true;
            break;
        case ArrowItem::HeadDiamondFilledTriangle:
        case ArrowItem::HeadFilledDiamondFilledTriangle:
            hasArrow = true;
            hasDiamond = true;
            break;
        }

        if (hasArrow) {
            if (!m_arrowItem)
                m_arrowItem = new QGraphicsPathItem(this);

            if (m_head == ArrowItem::HeadOpen || m_head == ArrowItem::HeadTriangle) {
                m_arrowItem->setPen(style->linePen());
                m_arrowItem->setBrush(QBrush());
            } else {
                m_arrowItem->setPen(style->linePen());
                m_arrowItem->setBrush(style->fillBrush());
            }

            QPainterPath path;
            double h = calcArrowLength();
            path.moveTo(-h, -m_arrowSize * 0.5);
            path.lineTo(0.0, 0.0);
            path.lineTo(-h, m_arrowSize * 0.5);
            if (m_head != ArrowItem::HeadOpen)
                path.closeSubpath();
            if (hasDiamond)
                path.translate(-calcDiamondLength(), 0.0);
            m_arrowItem->setPath(path);
        } else if (m_arrowItem) {
            m_arrowItem->scene()->removeItem(m_arrowItem);
            delete m_arrowItem;
            m_arrowItem = 0;
        }

        if (hasDiamond) {
            if (!m_diamondItem)
                m_diamondItem = new QGraphicsPathItem(this);

            if (m_head == ArrowItem::HeadDiamond || m_head == ArrowItem::HeadDiamondFilledTriangle) {
                m_diamondItem->setPen(style->linePen());
                m_diamondItem->setBrush(QBrush());
            } else {
                m_diamondItem->setPen(style->linePen());
                m_diamondItem->setBrush(style->fillBrush());
            }

            QPainterPath path;
            double h = calcDiamondLength();
            path.lineTo(-h * 0.5, -m_diamondSize * 0.5);
            path.lineTo(-h, 0.0);
            path.lineTo(-h * 0.5, m_diamondSize * 0.5);
            path.closeSubpath();
            m_diamondItem->setPath(path);
        } else if (m_diamondItem) {
            m_diamondItem->scene()->removeItem(m_diamondItem);
            delete m_diamondItem;
            m_diamondItem = 0;
        }
    }

private:
    ArrowItem::Head m_head = ArrowItem::HeadNone;
    double m_arrowSize = 10.0;
    double m_diamondSize = 15.0;
    QGraphicsPathItem *m_arrowItem = 0;
    QGraphicsPathItem *m_diamondItem = 0;
};

class ArrowItem::GraphicsShaftItem : public QGraphicsPathItem
{
public:
    explicit GraphicsShaftItem(QGraphicsItem *parent)
        : QGraphicsPathItem(parent)
    {
    }
};

ArrowItem::ArrowItem(QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_shaft(ShaftSolid),
      m_shaftItem(new GraphicsShaftItem(this))
{
}

ArrowItem::ArrowItem(const ArrowItem &rhs, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_shaft(rhs.m_shaft),
      m_shaftItem(new GraphicsShaftItem(this)),
      m_arrowSize(rhs.m_arrowSize),
      m_diamondSize(rhs.m_diamondSize),
      m_startHead(rhs.m_startHead),
      m_startHeadItem(0),
      m_endHead(rhs.m_endHead),
      m_endHeadItem(0)
{
}

ArrowItem::~ArrowItem()
{
}

void ArrowItem::setShaft(ArrowItem::Shaft shaft)
{
    if (m_shaft != shaft)
        m_shaft = shaft;
}

void ArrowItem::setArrowSize(double arrowSize)
{
    if (m_arrowSize != arrowSize)
        m_arrowSize = arrowSize;
}

void ArrowItem::setDiamondSize(double diamondSize)
{
    if (m_diamondSize != diamondSize)
        m_diamondSize = diamondSize;
}

void ArrowItem::setStartHead(ArrowItem::Head head)
{
    if (m_startHead != head)
        m_startHead = head;
}

void ArrowItem::setEndHead(ArrowItem::Head head)
{
    if (m_endHead != head)
        m_endHead = head;
}

void ArrowItem::setPoints(const QList<QPointF> &points)
{
    m_points = points;
}

QRectF ArrowItem::boundingRect() const
{
    return childrenBoundingRect();
}

void ArrowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);

#ifdef DEBUG_PAINT_SHAPE
    painter->setPen(QPen(Qt::blue));
    painter->drawRect(boundingRect());
    painter->setPen(QPen(Qt::red));
    painter->drawPath(shape());
    painter->setPen(QPen(Qt::green));
    if (m_startHeadItem)
        painter->drawRect(mapRectFromItem(m_startHeadItem, m_startHeadItem->boundingRect()));
    if (m_endHeadItem)
        painter->drawRect(mapRectFromItem(m_endHeadItem, m_endHeadItem->boundingRect()));
#endif
}

QPainterPath ArrowItem::shape() const
{
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    if (m_shaftItem &&m_shaftItem->path() != QPainterPath()) {
        QPainterPathStroker ps;
        QPen pen = m_shaftItem->pen();
        ps.setCapStyle(pen.capStyle());
        ps.setJoinStyle(pen.joinStyle());
        ps.setMiterLimit(pen.miterLimit());
        // overwrite pen width to make selection more lazy
        ps.setWidth(16.0);
        QPainterPath p = ps.createStroke(m_shaftItem->path());
        path.addPath(p);
    }
    if (m_startHeadItem)
        path.addRect(mapRectFromItem(m_startHeadItem, m_startHeadItem->boundingRect()));
    if (m_endHeadItem)
        path.addRect(mapRectFromItem(m_endHeadItem, m_endHeadItem->boundingRect()));
    return path;
}

QPointF ArrowItem::calcPointAtPercent(double percentage) const
{
    return m_shaftItem->path().pointAtPercent(percentage);
}

QLineF ArrowItem::firstLineSegment() const
{
    QTC_ASSERT(m_points.size() > 1, return QLineF());
    return QLineF(m_points.at(0), m_points.at(1));
}

QLineF ArrowItem::lastLineSegment() const
{
    QTC_ASSERT(m_points.size() > 1, return QLineF());
    return QLineF(m_points.at(m_points.size() - 1), m_points.at(m_points.size() - 2));
}

double ArrowItem::startHeadLength() const
{
    if (m_startHeadItem)
        return m_startHeadItem->calcHeadLength();
    return 0.0;
}

double ArrowItem::endHeadLength() const
{
    if (m_endHeadItem)
        return m_endHeadItem->calcHeadLength();
    return 0.0;
}

void ArrowItem::update(const Style *style)
{
    updateShaft(style);
    updateHead(&m_startHeadItem, m_startHead, style);
    updateHead(&m_endHeadItem, m_endHead, style);
    updateGeometry();
}

void ArrowItem::updateShaft(const Style *style)
{
    QTC_ASSERT(m_shaftItem, return);

    QPen pen(style->linePen());
    if (m_shaft == ShaftDashed)
        pen.setDashPattern(QVector<qreal>() << (4.0 / pen.widthF()) << (4.0 / pen.widthF()));
    m_shaftItem->setPen(pen);
}

void ArrowItem::updateHead(GraphicsHeadItem **headItem, Head head, const Style *style)
{
    if (head == HeadNone) {
        if (*headItem) {
            if ((*headItem)->scene())
                (*headItem)->scene()->removeItem(*headItem);
            delete *headItem;
            *headItem = 0;
        }
        return;
    }
    if (!*headItem)
        *headItem = new GraphicsHeadItem(this);
    (*headItem)->setArrowSize(m_arrowSize);
    (*headItem)->setDiamondSize(m_diamondSize);
    (*headItem)->setHead(head);
    (*headItem)->update(style);
}

void ArrowItem::updateHeadGeometry(GraphicsHeadItem **headItem, const QPointF &pos, const QPointF &otherPos)
{
    if (!*headItem)
        return;

    (*headItem)->setPos(pos);

    QVector2D directionVector(pos - otherPos);
    directionVector.normalize();
    double angle = qAcos(directionVector.x()) * 180.0 / 3.1415926535;
    if (directionVector.y() > 0.0)
        angle = -angle;
    (*headItem)->setRotation(-angle);
}

void ArrowItem::updateGeometry()
{
    QTC_ASSERT(m_points.size() > 1, return);
    QTC_ASSERT(m_shaftItem, return);

    prepareGeometryChange();

    QPainterPath path;

    if (m_startHeadItem) {
        QVector2D startDirectionVector(m_points.at(1) - m_points.at(0));
        startDirectionVector.normalize();
        startDirectionVector *= m_startHeadItem->calcHeadLength();
        path.moveTo(m_points.at(0) + startDirectionVector.toPointF());
    } else {
        path.moveTo(m_points.at(0));
    }

    for (int i = 1; i < m_points.size() - 1; ++i)
        path.lineTo(m_points.at(i));

    if (m_endHeadItem) {
        QVector2D endDirectionVector(m_points.at(m_points.size() - 1) - m_points.at(m_points.size() - 2));
        endDirectionVector.normalize();
        endDirectionVector *= m_endHeadItem->calcHeadLength();
        path.lineTo(m_points.at(m_points.size() - 1) - endDirectionVector.toPointF());
    } else {
        path.lineTo(m_points.at(m_points.size() - 1));
    }

    m_shaftItem->setPath(path);

    updateHeadGeometry(&m_startHeadItem, m_points.at(0), m_points.at(1));
    updateHeadGeometry(&m_endHeadItem, m_points.at(m_points.size() - 1), m_points.at(m_points.size() - 2));
}

} // namespace qmt
