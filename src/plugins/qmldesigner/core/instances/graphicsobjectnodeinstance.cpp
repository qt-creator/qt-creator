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

#include "graphicsobjectnodeinstance.h"

#include "invalidnodeinstanceexception.h"
#include <QGraphicsObject>
#include "nodemetainfo.h"

namespace QmlDesigner {
namespace Internal {

GraphicsObjectNodeInstance::GraphicsObjectNodeInstance(QGraphicsObject *graphicsObject, bool hasContent)
   : ObjectNodeInstance(graphicsObject),
   m_hasContent(hasContent)
{
}

QGraphicsObject *GraphicsObjectNodeInstance::graphicsObject() const
{
    Q_ASSERT(qobject_cast<QGraphicsObject*>(object()));
    return static_cast<QGraphicsObject*>(object());
}

bool GraphicsObjectNodeInstance::hasContent() const
{
    return m_hasContent;
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
    if (graphicsObject()->parentItem())
        return graphicsObject()->itemTransform(graphicsObject()->parentItem());
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

QRectF GraphicsObjectNodeInstance::boundingRect() const
{
    return graphicsObject()->boundingRect();
}

bool GraphicsObjectNodeInstance::isTopLevel() const
{
    Q_ASSERT(graphicsObject());
    return !graphicsObject()->parentItem();
}

bool GraphicsObjectNodeInstance::isGraphicsObject() const
{
    return true;
}

void GraphicsObjectNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    ObjectNodeInstance::setPropertyVariant(name, value);
}

QVariant GraphicsObjectNodeInstance::property(const QString &name) const
{
    return ObjectNodeInstance::property(name);
}

bool GraphicsObjectNodeInstance::equalGraphicsItem(QGraphicsItem *item) const
{
    return item == graphicsObject();
}

void GraphicsObjectNodeInstance::paintRecursively(QGraphicsItem *graphicsItem, QPainter *painter) const
{
    if (graphicsItem->isVisible()) {
        painter->save();
        painter->setTransform(graphicsItem->itemTransform(graphicsItem->parentItem()), true);
        painter->setOpacity(graphicsItem->opacity() * painter->opacity());
        graphicsItem->paint(painter, 0);
        foreach(QGraphicsItem *childItem, graphicsItem->childItems()) {
            paintRecursively(childItem, painter);
        }
        painter->restore();
    }
}

void GraphicsObjectNodeInstance::paint(QPainter *painter) const
{
    painter->save();
    Q_ASSERT(graphicsObject());
    if (hasContent())
        graphicsObject()->paint(painter, 0);

    foreach(QGraphicsItem *graphicsItem, graphicsObject()->childItems()) {
        QGraphicsObject *graphicsObject = qgraphicsitem_cast<QGraphicsObject*>(graphicsItem);
        if (graphicsObject && !nodeInstanceView()->hasInstanceForObject(graphicsObject))
            paintRecursively(graphicsItem, painter);
    }


    painter->restore();
}

QPair<QGraphicsObject*, bool> GraphicsObjectNodeInstance::createGraphicsObject(const NodeMetaInfo &metaInfo, QDeclarativeContext *context)
{
    QObject *object = ObjectNodeInstance::createObject(metaInfo, context);
    QGraphicsObject *graphicsObject = qobject_cast<QGraphicsObject*>(object);

    if (graphicsObject == 0)
        throw InvalidNodeInstanceException(__LINE__, __FUNCTION__, __FILE__);

//    graphicsObject->setCacheMode(QGraphicsItem::ItemCoordinateCache);
    bool hasContent = !graphicsObject->flags().testFlag(QGraphicsItem::ItemHasNoContents) || metaInfo.isComponent();
    graphicsObject->setFlag(QGraphicsItem::ItemHasNoContents, false);

    return qMakePair(graphicsObject, hasContent);
}

void GraphicsObjectNodeInstance::paintUpdate()
{
    graphicsObject()->update();
}
} // namespace Internal
} // namespace QmlDesigner
