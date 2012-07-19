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

#include "anchorlinehandleitem.h"

#include <formeditoritem.h>
#include <QPen>
#include <cmath>

namespace QmlDesigner {

AnchorLineHandleItem::AnchorLineHandleItem(QGraphicsItem *parent, const AnchorLineController &anchorLineController)
    : QGraphicsPathItem(parent),
    m_anchorLineControllerData(anchorLineController.weakPointer())
{
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setHiglighted(false);
}

void AnchorLineHandleItem::setHandlePath(const QPainterPath & path)
{
    setPath(path);
    update();
}

QRectF AnchorLineHandleItem::boundingRect() const
{
    return QGraphicsPathItem::boundingRect();
}

QPainterPath AnchorLineHandleItem::shape() const
{
    return QGraphicsPathItem::shape();
}

AnchorLineController AnchorLineHandleItem::anchorLineController() const
{
    Q_ASSERT(!m_anchorLineControllerData.isNull());
    return AnchorLineController(m_anchorLineControllerData.toStrongRef());
}

AnchorLine::Type AnchorLineHandleItem::anchorLine() const
{
    if (isTopHandle())
        return AnchorLine::Top;

    if (isLeftHandle())
        return AnchorLine::Left;

    if (isRightHandle())
        return AnchorLine::Right;

     if (isBottomHandle())
        return AnchorLine::Bottom;

    return AnchorLine::Invalid;
}

AnchorLineHandleItem* AnchorLineHandleItem::fromGraphicsItem(QGraphicsItem *item)
{
    return qgraphicsitem_cast<AnchorLineHandleItem*>(item);
}

bool AnchorLineHandleItem::isTopHandle() const
{
    return anchorLineController().isTopHandle(this);
}

bool AnchorLineHandleItem::isLeftHandle() const
{
    return anchorLineController().isLeftHandle(this);
}

bool AnchorLineHandleItem::isRightHandle() const
{
    return anchorLineController().isRightHandle(this);
}

bool AnchorLineHandleItem::isBottomHandle() const
{
    return anchorLineController().isBottomHandle(this);
}

AnchorLine::Type AnchorLineHandleItem::anchorLineType() const
{
    if (isTopHandle())
        return AnchorLine::Top;

    if (isBottomHandle())
        return AnchorLine::Bottom;

    if (isLeftHandle())
        return AnchorLine::Left;

    if (isRightHandle())
        return AnchorLine::Right;


    return AnchorLine::Invalid;
}

QPointF AnchorLineHandleItem::itemSpacePosition() const
{
    return parentItem()->mapToItem(anchorLineController().formEditorItem(), pos());
}

void AnchorLineHandleItem::setHiglighted(bool highlight)
{
    if (highlight) {
        QPen pen;
        pen.setColor(QColor(108, 141, 221));
        setPen(pen);
        setBrush(QColor(108, 141, 221, 140));
    } else {
        QPen pen;
        pen.setColor(QColor(108, 141, 221));
        setPen(pen);
        setBrush(QColor(108, 141, 221, 60));
    }
}

} // namespace QmlDesigner
