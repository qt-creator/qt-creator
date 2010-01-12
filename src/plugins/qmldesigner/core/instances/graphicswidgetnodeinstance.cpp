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

#include "graphicswidgetnodeinstance.h"
#include "graphicsscenenodeinstance.h"

#include "objectnodeinstance.h"

#include <QmlMetaType>

#include <invalidnodeinstanceexception.h>
#include <propertymetainfo.h>

namespace QmlDesigner {
namespace Internal {

GraphicsWidgetNodeInstance::GraphicsWidgetNodeInstance(QGraphicsWidget *widget)
   : ObjectNodeInstance(widget)
{
}

GraphicsWidgetNodeInstance::~GraphicsWidgetNodeInstance()
{
}

GraphicsWidgetNodeInstance::Pointer GraphicsWidgetNodeInstance::create(const NodeMetaInfo &nodeMetaInfo, QmlContext *context, QObject *objectToBeWrapped)
{
    QObject *object = 0;
    if (objectToBeWrapped)
        object = objectToBeWrapped;
    else
        object = createObject(nodeMetaInfo, context);

    QGraphicsWidget* graphicsWidget = qobject_cast<QGraphicsWidget*>(object);
    if (graphicsWidget == 0)
        throw InvalidNodeInstanceException(__LINE__, __FUNCTION__, __FILE__);

    Pointer instance(new GraphicsWidgetNodeInstance(graphicsWidget));

    if (objectToBeWrapped)
        instance->setDeleteHeldInstance(false); // the object isn't owned

    instance->populateResetValueHash();

    return instance;
}

void GraphicsWidgetNodeInstance::paint(QPainter *painter) const
{
    graphicsWidget()->show();
    graphicsWidget()->paint(painter, 0);

    paintChildren(painter, graphicsWidget());
}

bool GraphicsWidgetNodeInstance::isTopLevel() const
{
    return !graphicsWidget()->parentItem();
}


bool isChildNode(QGraphicsItem *item)
{
    // there should be a better implementation
    if (qgraphicsitem_cast<QGraphicsWidget*>(item))
        return false;
    else
        return true;
}

void GraphicsWidgetNodeInstance::paintChildren(QPainter *painter, QGraphicsItem *item) const
{
    foreach (QGraphicsItem *childItem, item->childItems()) {
        if (!isChildNode(childItem)) {
            painter->save();
            painter->setTransform(item->transform(), true);
            item->paint(painter, 0);
            paintChildren(painter, childItem);
            painter->restore();
        }
    }
}

void GraphicsWidgetNodeInstance::setParentItem(QGraphicsItem *item)
{
    graphicsWidget()->setParentItem(item);
}

QGraphicsWidget* GraphicsWidgetNodeInstance::graphicsWidget() const
{
    return static_cast<QGraphicsWidget*>(object());
}

bool GraphicsWidgetNodeInstance::isGraphicsWidget() const
{
    return true;
}

bool GraphicsWidgetNodeInstance::isGraphicsItem(QGraphicsItem *item) const
{
    return graphicsWidget() == item;
}

QRectF GraphicsWidgetNodeInstance::boundingRect() const
{
    return graphicsWidget()->boundingRect();
}

void GraphicsWidgetNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    if (name == "x")
        graphicsWidget()->setPos(value.toDouble(), graphicsWidget()->y());
    else if (name == "y")
        graphicsWidget()->setPos(graphicsWidget()->x(), value.toDouble());
    else if (name == "width")
        graphicsWidget()->resize(value.toDouble(), graphicsWidget()->size().height());
    else if (name == "height")
        graphicsWidget()->resize(graphicsWidget()->size().width(), value.toDouble());
    else
        graphicsWidget()->setProperty(name.toLatin1(), value);
}

QVariant GraphicsWidgetNodeInstance::property(const QString &name) const
{
    return graphicsWidget()->property(name.toLatin1());
}

bool GraphicsWidgetNodeInstance::isVisible() const
{
    return graphicsWidget()->isVisible();
}

void GraphicsWidgetNodeInstance::setVisible(bool isVisible)
{
    graphicsWidget()->setVisible(isVisible);
}

QPointF GraphicsWidgetNodeInstance::position() const
{
    return graphicsWidget()->pos();
}

QSizeF GraphicsWidgetNodeInstance::size() const
{
    return graphicsWidget()->size();
}

QTransform GraphicsWidgetNodeInstance::transform() const
{
    return graphicsWidget()->transform();
}

double GraphicsWidgetNodeInstance::opacity() const
{
    return graphicsWidget()->opacity();
}

}
}
