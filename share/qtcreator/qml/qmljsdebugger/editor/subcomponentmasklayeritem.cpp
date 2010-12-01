/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "subcomponentmasklayeritem.h"
#include "qmlobserverconstants.h"
#include "qdeclarativeviewobserver.h"
#include <QPolygonF>

namespace QmlJSDebugger {

SubcomponentMaskLayerItem::SubcomponentMaskLayerItem(QDeclarativeViewObserver *observer,
                                                     QGraphicsItem *parentItem) :
    QGraphicsPolygonItem(parentItem),
    m_observer(observer),
    m_currentItem(0),
    m_borderRect(new QGraphicsRectItem(this))
{
    m_borderRect->setRect(0,0,0,0);
    m_borderRect->setPen(QPen(QColor(60, 60, 60), 1));
    m_borderRect->setData(Constants::EditorItemDataKey, QVariant(true));

    setBrush(QBrush(QColor(160,160,160)));
    setPen(Qt::NoPen);
}

int SubcomponentMaskLayerItem::type() const
{
    return Constants::EditorItemType;
}

static QRectF resizeRect(const QRectF &newRect, const QRectF &oldRect)
{
    QRectF result = newRect;
    if (oldRect.left() < newRect.left())
        result.setLeft(oldRect.left());

    if (oldRect.top() < newRect.top())
        result.setTop(oldRect.top());

    if (oldRect.right() > newRect.right())
        result.setRight(oldRect.right());

    if (oldRect.bottom() > newRect.bottom())
        result.setBottom(oldRect.bottom());

    return result;
}


void SubcomponentMaskLayerItem::setCurrentItem(QGraphicsItem *item)
{
    QGraphicsItem *prevItem = m_currentItem;
    m_currentItem = item;

    if (!m_currentItem)
        return;

    QPolygonF viewPoly(QRectF(m_observer->declarativeView()->rect()));
    viewPoly = m_observer->declarativeView()->mapToScene(viewPoly.toPolygon());

    QRectF itemRect = item->boundingRect() | item->childrenBoundingRect();
    QPolygonF itemPoly(itemRect);
    itemPoly = item->mapToScene(itemPoly);

    // if updating the same item as before, resize the rectangle only bigger, not smaller.
    if (prevItem == item && prevItem != 0) {
        m_itemPolyRect = resizeRect(itemPoly.boundingRect(), m_itemPolyRect);
    } else {
        m_itemPolyRect = itemPoly.boundingRect();
    }
    QRectF borderRect = m_itemPolyRect;
    borderRect.adjust(-1, -1, 1, 1);
    m_borderRect->setRect(borderRect);

    itemPoly = viewPoly.subtracted(QPolygonF(m_itemPolyRect));
    setPolygon(itemPoly);
}

QGraphicsItem *SubcomponentMaskLayerItem::currentItem() const
{
    return m_currentItem;
}

} // namespace QmlJSDebugger
