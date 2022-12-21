// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "layeritem.h"

#include <formeditorscene.h>

#include <utils/qtcassert.h>

#include <QGraphicsView>

namespace QmlDesigner {

LayerItem::LayerItem(FormEditorScene* scene)
{
    scene->addItem(this);
    setZValue(1);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setAcceptedMouseButtons(Qt::NoButton);
}

LayerItem::~LayerItem() = default;

void LayerItem::paint(QPainter * /*painter*/, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
}

QRectF LayerItem::boundingRect() const
{
    return childrenBoundingRect();
}

QList<QGraphicsItem*> LayerItem::findAllChildItems() const
{
    return findAllChildItems(this);
}

QTransform LayerItem::viewportTransform() const
{
    QTC_ASSERT(scene(), return {});
    QTC_ASSERT(!scene()->views().isEmpty(), return {});

    return scene()->views().first()->viewportTransform();
}

QList<QGraphicsItem*> LayerItem::findAllChildItems(const QGraphicsItem *item) const
{
    QList<QGraphicsItem*> itemList(item->childItems());

    for (int end = itemList.length(), i = 0; i < end; ++i)
        itemList += findAllChildItems(itemList.at(i));

    return itemList;
}

}
