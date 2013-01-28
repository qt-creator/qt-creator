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

#include "graphicsobjectnodeinstance.h"

#include <QGraphicsObject>
#include "private/qgraphicsitem_p.h"
#include <private/qdeclarativemetatype_p.h>

#include <QStyleOptionGraphicsItem>
#include <QPixmap>
#include <QSizeF>

namespace QmlDesigner {
namespace Internal {

GraphicsObjectNodeInstance::GraphicsObjectNodeInstance(QGraphicsObject *graphicsObject)
   : ObjectNodeInstance(graphicsObject),
   m_isMovable(true)
{
    QGraphicsItemPrivate::get(graphicsObject)->sendParentChangeNotification = 1;
}

QGraphicsObject *GraphicsObjectNodeInstance::graphicsObject() const
{
    if (object() == 0)
        return 0;

    Q_ASSERT(qobject_cast<QGraphicsObject*>(object()));
    return static_cast<QGraphicsObject*>(object());
}

bool GraphicsObjectNodeInstance::childrenHasContent(QGraphicsItem *graphicsItem) const
{
    QGraphicsObject *graphicsObject = graphicsItem->toGraphicsObject();

    if (graphicsObject && !nodeInstanceServer()->hasInstanceForObject(graphicsObject) && !graphicsItem->flags().testFlag(QGraphicsItem::ItemHasNoContents))
        return true;

    foreach (QGraphicsItem *childItem, graphicsItem->childItems()) {
        bool childHasContent = childrenHasContent(childItem);
        if (childHasContent)
            return true;
    }

    return false;
}

bool GraphicsObjectNodeInstance::hasContent() const
{
    if (m_hasContent)
        return true;

    return childrenHasContent(graphicsObject());
}

QList<ServerNodeInstance> GraphicsObjectNodeInstance::childItems() const
{
    QList<ServerNodeInstance> instanceList;
    foreach(QGraphicsItem *item, graphicsObject()->childItems())
    {
        QGraphicsObject *childObject = item->toGraphicsObject();
        if (childObject && nodeInstanceServer()->hasInstanceForObject(childObject)) {
            instanceList.append(nodeInstanceServer()->instanceForObject(childObject));
        } else { //there might be an item in between the parent instance
                 //and the child instance.
                 //Popular example is flickable which has a viewport item between
                 //the flickable item and the flickable children
            instanceList.append(childItemsForChild(item)); //In such a case we go deeper inside the item and
                                                           //search for child items with instances.
        }
    }

    return instanceList;
}

QList<ServerNodeInstance> GraphicsObjectNodeInstance::childItemsForChild(QGraphicsItem *childItem) const
{
    QList<ServerNodeInstance> instanceList;
    if (childItem) {

        foreach(QGraphicsItem *item, childItem->childItems())
        {
            QGraphicsObject *childObject = item->toGraphicsObject();
            if (childObject && nodeInstanceServer()->hasInstanceForObject(childObject)) {
                instanceList.append(nodeInstanceServer()->instanceForObject(childObject));
            } else {
                instanceList.append(childItemsForChild(item));
            }
        }
    }
    return instanceList;
}

void GraphicsObjectNodeInstance::setHasContent(bool hasContent)
{
    m_hasContent = hasContent;
}

QPointF GraphicsObjectNodeInstance::position() const
{
    return graphicsObject()->pos();
}

QSizeF GraphicsObjectNodeInstance::size() const
{
    return graphicsObject()->boundingRect().size();
}

QTransform GraphicsObjectNodeInstance::transform() const
{
    if (!nodeInstanceServer()->hasInstanceForObject(object()))
        return sceneTransform();

    ServerNodeInstance nodeInstanceParent = nodeInstanceServer()->instanceForObject(object()).parent();

    if (!nodeInstanceParent.isValid())
        return sceneTransform();

    QGraphicsObject *graphicsObjectParent = qobject_cast<QGraphicsObject*>(nodeInstanceParent.internalObject());
    if (graphicsObjectParent)
        return graphicsObject()->itemTransform(graphicsObjectParent);
    else
        return sceneTransform();
}

QTransform GraphicsObjectNodeInstance::customTransform() const
{
    return graphicsObject()->transform();
}

QTransform GraphicsObjectNodeInstance::sceneTransform() const
{
    return graphicsObject()->sceneTransform();
}

double GraphicsObjectNodeInstance::rotation() const
{
    return graphicsObject()->rotation();
}

double GraphicsObjectNodeInstance::scale() const
{
    return graphicsObject()->scale();
}

QList<QGraphicsTransform *> GraphicsObjectNodeInstance::transformations() const
{
    return graphicsObject()->transformations();
}

QPointF GraphicsObjectNodeInstance::transformOriginPoint() const
{
    return graphicsObject()->transformOriginPoint();
}

double GraphicsObjectNodeInstance::zValue() const
{
    return graphicsObject()->zValue();
}

double GraphicsObjectNodeInstance::opacity() const
{
    return graphicsObject()->opacity();
}

QObject *GraphicsObjectNodeInstance::parent() const
{
    if (!graphicsObject() || !graphicsObject()->parentItem())
        return 0;

    return graphicsObject()->parentItem()->toGraphicsObject();
}

static inline bool isRectangleSane(const QRectF &rect)
{
    return rect.isValid() && (rect.width() < 10000) && (rect.height() < 10000);
}

QRectF GraphicsObjectNodeInstance::boundingRectWithStepChilds(QGraphicsItem *parentItem) const
{
    QRectF boundingRect = parentItem->boundingRect();

    foreach (QGraphicsItem *childItem, parentItem->childItems()) {
        QGraphicsObject *childObject = childItem->toGraphicsObject();
        if (!(childObject && nodeInstanceServer()->hasInstanceForObject(childObject))) {
            QRectF transformedRect = childItem->mapRectToParent(boundingRectWithStepChilds(childItem));
            if (isRectangleSane(transformedRect))
                boundingRect = boundingRect.united(transformedRect);
        }
    }

    return boundingRect;
}

QRectF GraphicsObjectNodeInstance::boundingRect() const
{
    if (graphicsObject()) {
        if (graphicsObject()->flags() & QGraphicsItem::ItemClipsChildrenToShape) {
            return graphicsObject()->boundingRect();
        } else {
            return boundingRectWithStepChilds(graphicsObject());
        }
    }

    return QRectF();
}

bool GraphicsObjectNodeInstance::isGraphicsObject() const
{
    return true;
}

void GraphicsObjectNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    ObjectNodeInstance::setPropertyVariant(name, value);
}

void GraphicsObjectNodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    ObjectNodeInstance::setPropertyBinding(name, expression);
}

QVariant GraphicsObjectNodeInstance::property(const QString &name) const
{
    return ObjectNodeInstance::property(name);
}

bool GraphicsObjectNodeInstance::equalGraphicsItem(QGraphicsItem *item) const
{
    return item == graphicsObject();
}

void initOption(QGraphicsItem *item, QStyleOptionGraphicsItem *option, const QTransform &transform)
{
    QGraphicsItemPrivate *privateItem = QGraphicsItemPrivate::get(item);
    privateItem->initStyleOption(option, transform, QRegion());
}

QImage GraphicsObjectNodeInstance::renderImage() const
{
    QRectF boundingRect = GraphicsObjectNodeInstance::boundingRect();
    QSize boundingSize = boundingRect.size().toSize();

    QImage image(boundingSize, QImage::Format_ARGB32_Premultiplied);

    if (image.isNull())
        return image;

    image.fill(0x00000000);

    QPainter painter(&image);
    painter.translate(-boundingRect.topLeft());

    if (hasContent()) {
        QStyleOptionGraphicsItem option;
        initOption(graphicsObject(), &option, painter.transform());
        graphicsObject()->paint(&painter, &option);
    }

    foreach(QGraphicsItem *graphicsItem, graphicsObject()->childItems())
        paintRecursively(graphicsItem, &painter);

    return image;
}

void GraphicsObjectNodeInstance::paintRecursively(QGraphicsItem *graphicsItem, QPainter *painter) const
{
    QGraphicsObject *graphicsObject = graphicsItem->toGraphicsObject();
    if (graphicsObject) {
        if (nodeInstanceServer()->hasInstanceForObject(graphicsObject))
            return; //we already keep track of this object elsewhere
    }

    if (graphicsItem->isVisible()) {
        painter->save();
        painter->setTransform(graphicsItem->itemTransform(graphicsItem->parentItem()), true);
        painter->setOpacity(graphicsItem->opacity() * painter->opacity());
        QStyleOptionGraphicsItem option;
        initOption(graphicsItem, &option, painter->transform());
        graphicsItem->paint(painter, &option);
        foreach(QGraphicsItem *childItem, graphicsItem->childItems()) {
            paintRecursively(childItem, painter);
        }
        painter->restore();
    }
}

void GraphicsObjectNodeInstance::paintUpdate()
{
    graphicsObject()->update();
}

bool GraphicsObjectNodeInstance::isMovable() const
{
    if (isRootNodeInstance())
        return false;

    return m_isMovable && graphicsObject() && graphicsObject()->parentItem();
}

void GraphicsObjectNodeInstance::setMovable(bool movable)
{
    m_isMovable = movable;
}

} // namespace Internal
} // namespace QmlDesigner
