/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "resizehandleitem.h"

#include <QPainter>

namespace QmlDesigner {

ResizeHandleItem::ResizeHandleItem(QGraphicsItem *parent, const ResizeController &resizeController)
    : QGraphicsItem(parent),
    m_weakResizeController(resizeController.toWeakResizeController())
{
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
}

ResizeHandleItem::~ResizeHandleItem()
{
}

void ResizeHandleItem::setHandlePosition(const QPointF & globalPosition, const QPointF & itemSpacePosition)
{
    m_itemSpacePosition = itemSpacePosition;
    setPos(globalPosition);
}

QRectF ResizeHandleItem::boundingRect() const
{
    return QRectF(- 3., - 3., 7., 7.);
}

void ResizeHandleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /* option */, QWidget * /* widget */)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setBrush(QColor(255, 255, 255));
    painter->drawRect(QRectF(-2., -2., 4., 4.));

    painter->restore();
}


ResizeController ResizeHandleItem::resizeController() const
{
    return ResizeController(m_weakResizeController.toResizeController());
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
