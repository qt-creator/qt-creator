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

#include "selectionindicator.h"

#include <QPen>
#include <cmath>
#include <QGraphicsScene>
#include <formeditorview.h>
#include <formeditorwidget.h>
#include <zoomaction.h>

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
    foreach (QGraphicsPolygonItem *item, m_indicatorShapeHash.values())
        item->show();
}

void SelectionIndicator::hide()
{
    foreach (QGraphicsPolygonItem *item, m_indicatorShapeHash.values())
        item->hide();
}

void SelectionIndicator::clear()
{
    if (m_layerItem) {
        foreach (QGraphicsItem *item, m_indicatorShapeHash.values())
            m_layerItem->scene()->removeItem(item);
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

    foreach (QGraphicsItem  *item, m_indicatorShapeHash.values())
        item->setCursor(cursor);
}

}

