/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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
