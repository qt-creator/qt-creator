/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "liveselectionindicator.h"
#include "../qdeclarativeviewinspector_p.h"
#include "qmlinspectorconstants.h"

#include <QtGui/QGraphicsRectItem>
#include <QtGui/QGraphicsObject>
#include <QtGui/QGraphicsScene>
#include <QtGui/QPen>

namespace QmlJSDebugger {

LiveSelectionIndicator::LiveSelectionIndicator(QDeclarativeViewInspector *viewInspector,
                                               QGraphicsObject *layerItem)
    : m_layerItem(layerItem)
    , m_view(viewInspector)
{
}

LiveSelectionIndicator::~LiveSelectionIndicator()
{
    clear();
}

void LiveSelectionIndicator::show()
{
    foreach (QGraphicsRectItem *item, m_indicatorShapeHash)
        item->show();
}

void LiveSelectionIndicator::hide()
{
    foreach (QGraphicsRectItem *item, m_indicatorShapeHash)
        item->hide();
}

void LiveSelectionIndicator::clear()
{
    if (!m_layerItem.isNull()) {
        QGraphicsScene *scene = m_layerItem.data()->scene();
        foreach (QGraphicsRectItem *item, m_indicatorShapeHash) {
            scene->removeItem(item);
            delete item;
        }
    }

    m_indicatorShapeHash.clear();

}

void LiveSelectionIndicator::setItems(const QList<QWeakPointer<QGraphicsObject> > &itemList)
{
    clear();

    foreach (const QWeakPointer<QGraphicsObject> &object, itemList) {
        if (object.isNull())
            continue;

        QGraphicsItem *item = object.data();

        if (!m_indicatorShapeHash.contains(item)) {
            QGraphicsRectItem *selectionIndicator = new QGraphicsRectItem(m_layerItem.data());
            m_indicatorShapeHash.insert(item, selectionIndicator);

            const QRectF boundingRect = m_view->adjustToScreenBoundaries(item->mapRectToScene(item->boundingRect()));
            const QRectF boundingRectInLayerItemSpace = m_layerItem.data()->mapRectFromScene(boundingRect);

            selectionIndicator->setData(Constants::EditorItemDataKey, true);
            selectionIndicator->setFlag(QGraphicsItem::ItemIsSelectable, false);
            selectionIndicator->setRect(boundingRectInLayerItemSpace);
            selectionIndicator->setPen(QColor(108, 141, 221));
        }
    }
}

} //namespace QmlJSDebugger

