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

#include "graphicsscenenodeinstance.h"

#include <private/qdeclarativemetatype_p.h>

#include "graphicsviewnodeinstance.h"

#include <invalidnodeinstanceexception.h>
#include <propertymetainfo.h>

namespace QmlDesigner {
namespace Internal {

GraphicsSceneNodeInstance::GraphicsSceneNodeInstance(QGraphicsScene *scene)
   :ObjectNodeInstance(scene)
{
}

GraphicsSceneNodeInstance::~GraphicsSceneNodeInstance()
{
}

GraphicsSceneNodeInstance::Pointer GraphicsSceneNodeInstance::create(const NodeMetaInfo &nodeMetaInfo, QDeclarativeContext *context, QObject  *objectToBeWrapped)
{
    QObject *object = 0;
    if (objectToBeWrapped)
        object = objectToBeWrapped;
    else
        object = createObject(nodeMetaInfo, context);

    QGraphicsScene* scene = qobject_cast<QGraphicsScene*>(object);
    if (scene == 0)
        throw InvalidNodeInstanceException(__LINE__, __FUNCTION__, __FILE__);

    Pointer instance(new GraphicsSceneNodeInstance(scene));

    if (objectToBeWrapped)
        instance->setDeleteHeldInstance(false); // the object isn't owned

    instance->populateResetValueHash();

    return instance;
}

void GraphicsSceneNodeInstance::paint(QPainter *) const
{
    Q_ASSERT(graphicsScene());
}

bool GraphicsSceneNodeInstance::isTopLevel() const
{
    Q_ASSERT(graphicsScene());
    return graphicsScene()->views().isEmpty();
}


void GraphicsSceneNodeInstance::addItem(QGraphicsItem *item)
{
    graphicsScene()->addItem(item);
}

bool GraphicsSceneNodeInstance::isGraphicsScene() const
{
    return true;
}

QRectF GraphicsSceneNodeInstance::boundingRect() const
{
    return graphicsScene()->sceneRect();
}

QPointF GraphicsSceneNodeInstance::position() const
{
    return graphicsScene()->sceneRect().topLeft();
}

QSizeF GraphicsSceneNodeInstance::size() const
{
    return graphicsScene()->sceneRect().size();
}

QGraphicsScene *GraphicsSceneNodeInstance::graphicsScene() const
{
    Q_ASSERT(qobject_cast<QGraphicsScene*>(object()));
    return static_cast<QGraphicsScene*>(object());
}

bool GraphicsSceneNodeInstance::isVisible() const
{
    return false;
}

void GraphicsSceneNodeInstance::setVisible(bool /*isVisible*/)
{

}


QList<NodeInstance> GraphicsSceneNodeInstance::instancesInRegions(const QList<QRectF> &regions)
{
    QRectF combinedRect;
    foreach (const QRectF &rect, regions) {
        combinedRect = combinedRect.united(rect);
    }

    QList<QGraphicsItem*> itemList;
    // collect list of possibly changed items
    // (actually QGraphicsScene could export this)

    foreach (QGraphicsItem *item, graphicsScene()->items(combinedRect.adjusted(0, 0, 1, 1), Qt::ContainsItemBoundingRect)) {
        if (!itemList.contains(item))
            itemList.append(item);
    }

    QList<NodeInstance> instances;
    if (!itemList.isEmpty()) {
        foreach (const NodeInstance &nodeInstance, nodeInstanceView()->instances()) {
            foreach (QGraphicsItem *gvItem, itemList) {
                if (nodeInstance.equalGraphicsItem(gvItem)) {
                    instances += nodeInstance;
                }
            }
        }
    }

    return instances;
}

}
}
