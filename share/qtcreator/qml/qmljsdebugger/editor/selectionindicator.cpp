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

#include "selectionindicator.h"
#include "../qdeclarativeviewobserver_p.h"
#include "qmlobserverconstants.h"

#include <QPen>
#include <cmath>
#include <QGraphicsScene>
#include <QDebug>

namespace QmlJSDebugger {

SelectionIndicator::SelectionIndicator(QDeclarativeViewObserver *editorView, QGraphicsObject *layerItem)
    : m_layerItem(layerItem), m_view(editorView)
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
    if (!m_layerItem.isNull()) {
        QHashIterator<QGraphicsItem*, QGraphicsPolygonItem *> iter(m_indicatorShapeHash);
        while(iter.hasNext()) {
            iter.next();
            m_layerItem.data()->scene()->removeItem(iter.value());
            delete iter.value();
        }
    }

    m_indicatorShapeHash.clear();

}

QPolygonF SelectionIndicator::addBoundingRectToPolygon(QGraphicsItem *item, QPolygonF &polygon)
{
    // ### remove this if statement when QTBUG-12172 gets fixed
    if (item->boundingRect() != QRectF(0,0,0,0)) {
        QPolygonF bounding = item->mapToScene(item->boundingRect());
        if (bounding.isClosed()) //avoid crashes if there is an infinite scale.
            polygon = polygon.united(bounding);
    }

    foreach(QGraphicsItem *child, item->childItems()) {
        if (!QDeclarativeViewObserverPrivate::get(m_view)->isEditorItem(child))
            addBoundingRectToPolygon(child, polygon);
    }
    return polygon;
}

void SelectionIndicator::setItems(const QList<QWeakPointer<QGraphicsObject> > &itemList)
{
    clear();

    // set selections to also all children if they are not editor items

    foreach (QWeakPointer<QGraphicsObject> object, itemList) {
        if (object.isNull())
            continue;

        QGraphicsItem *item = object.data();

        QGraphicsPolygonItem *newSelectionIndicatorGraphicsItem = new QGraphicsPolygonItem(m_layerItem.data());
        if (!m_indicatorShapeHash.contains(item)) {
            m_indicatorShapeHash.insert(item, newSelectionIndicatorGraphicsItem);

            QPolygonF boundingShapeInSceneSpace;
            addBoundingRectToPolygon(item, boundingShapeInSceneSpace);

            QRectF boundingRect = m_view->adjustToScreenBoundaries(boundingShapeInSceneSpace.boundingRect());
            QPolygonF boundingRectInLayerItemSpace = m_layerItem.data()->mapFromScene(boundingRect);

            QPen pen;
            pen.setColor(QColor(108, 141, 221));
            newSelectionIndicatorGraphicsItem->setData(Constants::EditorItemDataKey, QVariant(true));
            newSelectionIndicatorGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
            newSelectionIndicatorGraphicsItem->setPolygon(boundingRectInLayerItemSpace);
            newSelectionIndicatorGraphicsItem->setPen(pen);
        }
    }
}

} //namespace QmlJSDebugger

