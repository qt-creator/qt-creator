/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "resizehandleitem.h"

#include <formeditoritem.h>
#include <QCursor>

namespace QmlDesigner {

ResizeHandleItem::ResizeHandleItem(QGraphicsItem *parent, const ResizeController &resizeController)
    : QGraphicsPixmapItem(QPixmap(":/icon/handle/resize_handle.png"), parent),
    m_resizeControllerData(resizeController.weakPointer())
{
    setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
    setOffset(-pixmap().rect().center());
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
}

void ResizeHandleItem::setHandlePosition(const QPointF & globalPosition, const QPointF & itemSpacePosition)
{
    m_itemSpacePosition = itemSpacePosition;
    setPos(globalPosition);
}

QRectF ResizeHandleItem::boundingRect() const
{
    return QGraphicsPixmapItem::boundingRect().adjusted(-1, -1, 1, 1);
}

QPainterPath ResizeHandleItem::shape() const
{
    return QGraphicsItem::shape();
}

ResizeController ResizeHandleItem::resizeController() const
{
    Q_ASSERT(!m_resizeControllerData.isNull());
    return ResizeController(m_resizeControllerData.toStrongRef());
}

ResizeHandleItem* ResizeHandleItem::fromGraphicsItem(QGraphicsItem *item)
{
    return qgraphicsitem_cast<ResizeHandleItem*>(item);
}

bool ResizeHandleItem::isTopLeftHandle() const
{
    return resizeController().isTopLeftHandle(this);
}

bool ResizeHandleItem::isTopRightHandle() const
{
    return resizeController().isTopRightHandle(this);
}

bool ResizeHandleItem::isBottomLeftHandle() const
{
    return resizeController().isBottomLeftHandle(this);
}

bool ResizeHandleItem::isBottomRightHandle() const
{
    return resizeController().isBottomRightHandle(this);
}

bool ResizeHandleItem::isTopHandle() const
{
    return resizeController().isTopHandle(this);
}

bool ResizeHandleItem::isLeftHandle() const
{
    return resizeController().isLeftHandle(this);
}

bool ResizeHandleItem::isRightHandle() const
{
    return resizeController().isRightHandle(this);
}

bool ResizeHandleItem::isBottomHandle() const
{
    return resizeController().isBottomHandle(this);
}

QPointF ResizeHandleItem::itemSpacePosition() const
{
    return m_itemSpacePosition;
}
}
