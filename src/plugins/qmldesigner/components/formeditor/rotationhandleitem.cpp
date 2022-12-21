// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rotationhandleitem.h"

#include <QPainter>
#include <QDebug>

namespace QmlDesigner {

RotationHandleItem::RotationHandleItem(QGraphicsItem *parent, const RotationController &rotationController)
    : QGraphicsItem(parent)
    , m_weakRotationController(rotationController.toWeakRotationController())
{
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    setAcceptedMouseButtons(Qt::NoButton);
}

RotationHandleItem::~RotationHandleItem() = default;

void RotationHandleItem::setHandlePosition(const QPointF & globalPosition, const QPointF & itemSpacePosition, const qreal rotation)
{
    m_itemSpacePosition = itemSpacePosition;
    setRotation(rotation);
    setPos(globalPosition);
}

QRectF RotationHandleItem::boundingRect() const
{
    QRectF rectangle;

    if (isTopLeftHandle()) {
        rectangle = { -30., -30., 27., 27.};
    }
    else if (isTopRightHandle()) {
        rectangle = { 3., -30., 27., 27.};
    }
    else if (isBottomLeftHandle()) {
        rectangle = { -30., 3., 27., 27.};
    }
    else if (isBottomRightHandle()) {
        rectangle = { 3., 3., 27., 27.};
    }
    return rectangle;
}

void RotationHandleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /* option */, QWidget * /* widget */)
{
    painter->save();
    QPen pen = painter->pen();
    pen.setWidth(1);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setBrush(QColor(255, 255, 255));
    painter->drawRect(QRectF(-3., -3., 5., 5.));

    painter->restore();
}


RotationController RotationHandleItem::rotationController() const
{
    return RotationController(m_weakRotationController.toRotationController());
}

RotationHandleItem* RotationHandleItem::fromGraphicsItem(QGraphicsItem *item)
{
    return qgraphicsitem_cast<RotationHandleItem*>(item);
}

bool RotationHandleItem::isTopLeftHandle() const
{
    return rotationController().isTopLeftHandle(this);
}

bool RotationHandleItem::isTopRightHandle() const
{
    return rotationController().isTopRightHandle(this);
}

bool RotationHandleItem::isBottomLeftHandle() const
{
    return rotationController().isBottomLeftHandle(this);
}

bool RotationHandleItem::isBottomRightHandle() const
{
    return rotationController().isBottomRightHandle(this);
}

QPointF RotationHandleItem::itemSpacePosition() const
{
    return m_itemSpacePosition;
}
}
