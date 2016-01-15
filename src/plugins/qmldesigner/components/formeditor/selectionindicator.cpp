/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "selectionindicator.h"

#include <QPen>
#include <QGraphicsScene>

namespace QmlDesigner {

SelectionIndicator::SelectionIndicator(LayerItem *layerItem)
    : m_layerItem(layerItem)
{
}

SelectionIndicator::~SelectionIndicator()
{
    clear();
}

void SelectionIndicator::show()
{
    foreach (QGraphicsPolygonItem *item, m_indicatorShapeHash)
        item->show();
}

void SelectionIndicator::hide()
{
    foreach (QGraphicsPolygonItem *item, m_indicatorShapeHash)
        item->hide();
}

void SelectionIndicator::clear()
{
    if (m_layerItem) {
        foreach (QGraphicsItem *item, m_indicatorShapeHash) {
            m_layerItem->scene()->removeItem(item);
            delete item;
        }
    }
    m_indicatorShapeHash.clear();
}

//static void alignVertices(QPolygonF &polygon, double factor)
//{
//    QMutableVectorIterator<QPointF> iterator(polygon);
//    while (iterator.hasNext()) {
//        QPointF &vertex = iterator.next();
//        vertex.setX(std::floor(vertex.x()) + factor);
//        vertex.setY(std::floor(vertex.y()) + factor);
//    }
//
//}

void SelectionIndicator::setItems(const QList<FormEditorItem*> &itemList)
{
    clear();

    foreach (FormEditorItem *item, itemList) {
        if (!item->qmlItemNode().isValid())
            continue;

        QGraphicsPolygonItem *newSelectionIndicatorGraphicsItem = new QGraphicsPolygonItem(m_layerItem.data());
        m_indicatorShapeHash.insert(item, newSelectionIndicatorGraphicsItem);
        QPolygonF boundingRectInSceneSpace(item->mapToScene(item->qmlItemNode().instanceBoundingRect()));
        //        alignVertices(boundingRectInSceneSpace, 0.5 / item->formEditorView()->widget()->zoomAction()->zoomLevel());
        QPolygonF boundingRectInLayerItemSpace = m_layerItem->mapFromScene(boundingRectInSceneSpace);
        newSelectionIndicatorGraphicsItem->setPolygon(boundingRectInLayerItemSpace);
        newSelectionIndicatorGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, false);

        QPen pen;
        pen.setColor(QColor(108, 141, 221));
        newSelectionIndicatorGraphicsItem->setPen(pen);
        newSelectionIndicatorGraphicsItem->setCursor(m_cursor);
    }
}



void SelectionIndicator::updateItems(const QList<FormEditorItem*> &itemList)
{
    foreach (FormEditorItem *item, itemList) {
        if (m_indicatorShapeHash.contains(item)) {
            QGraphicsPolygonItem *indicatorGraphicsItem =  m_indicatorShapeHash.value(item);
            QPolygonF boundingRectInSceneSpace(item->mapToScene(item->qmlItemNode().instanceBoundingRect()));
//            alignVertices(boundingRectInSceneSpace, 0.5 / item->formEditorView()->widget()->zoomAction()->zoomLevel());
            QPolygonF boundingRectInLayerItemSpace = m_layerItem->mapFromScene(boundingRectInSceneSpace);
            indicatorGraphicsItem->setPolygon(boundingRectInLayerItemSpace);
        }
    }
}

void SelectionIndicator::setCursor(const QCursor &cursor)
{
    m_cursor = cursor;

    foreach (QGraphicsItem  *item, m_indicatorShapeHash)
        item->setCursor(cursor);
}

}

