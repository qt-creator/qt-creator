/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "subcomponentmasklayeritem.h"
#include "qmlinspectorconstants.h"
#include "qdeclarativeviewinspector.h"
#include <QPolygonF>

namespace QmlJSDebugger {

SubcomponentMaskLayerItem::SubcomponentMaskLayerItem(QDeclarativeViewInspector *inspector,
                                                     QGraphicsItem *parentItem) :
    QGraphicsPolygonItem(parentItem),
    m_inspector(inspector),
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

static QPolygonF regionToPolygon(const QRegion &region)
{
    QPainterPath path;
    foreach (const QRect &rect, region.rects())
        path.addRect(rect);
    return path.toFillPolygon();
}

void SubcomponentMaskLayerItem::setCurrentItem(QGraphicsItem *item)
{
    QGraphicsItem *prevItem = m_currentItem;
    m_currentItem = item;

    if (!m_currentItem)
        return;

    QRect viewRect = m_inspector->declarativeView()->rect();
    viewRect = m_inspector->declarativeView()->mapToScene(viewRect).boundingRect().toRect();

    QRectF itemRect = item->boundingRect() | item->childrenBoundingRect();
    itemRect = item->mapRectToScene(itemRect);

    // if updating the same item as before, resize the rectangle only bigger, not smaller.
    if (prevItem == item && prevItem != 0) {
        m_itemPolyRect = resizeRect(itemRect, m_itemPolyRect);
    } else {
        m_itemPolyRect = itemRect;
    }
    QRectF borderRect = m_itemPolyRect;
    borderRect.adjust(-1, -1, 1, 1);
    m_borderRect->setRect(borderRect);

    const QRegion externalRegion = QRegion(viewRect).subtracted(m_itemPolyRect.toRect());
    setPolygon(regionToPolygon(externalRegion));
}

QGraphicsItem *SubcomponentMaskLayerItem::currentItem() const
{
    return m_currentItem;
}

} // namespace QmlJSDebugger
