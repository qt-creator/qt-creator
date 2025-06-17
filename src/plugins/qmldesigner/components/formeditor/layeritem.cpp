// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "layeritem.h"
#include "formeditortracing.h"

#include <formeditorscene.h>

#include <utils/qtcassert.h>

#include <QGraphicsView>

namespace QmlDesigner {

using FormEditorTracing::category;

LayerItem::LayerItem(FormEditorScene* scene)
{
    NanotraceHR::Tracer tracer{"layer item constructor", category()};

    scene->addItem(this);
    setZValue(1);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setAcceptedMouseButtons(Qt::NoButton);
}

LayerItem::~LayerItem() = default;

void LayerItem::paint(QPainter * /*painter*/, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    NanotraceHR::Tracer tracer{"layer item paint", category()};
}

QRectF LayerItem::boundingRect() const
{
    NanotraceHR::Tracer tracer{"layer item boundingRect", category()};

    return childrenBoundingRect();
}

QList<QGraphicsItem*> LayerItem::findAllChildItems() const
{
    NanotraceHR::Tracer tracer{"layer item findAllChildItems", category()};

    return findAllChildItems(this);
}

QTransform LayerItem::viewportTransform() const
{
    NanotraceHR::Tracer tracer{"layer item viewportTransform", category()};

    QTC_ASSERT(scene(), return {});
    QTC_ASSERT(!scene()->views().isEmpty(), return {});

    return scene()->views().first()->viewportTransform();
}

QList<QGraphicsItem*> LayerItem::findAllChildItems(const QGraphicsItem *item) const
{
    NanotraceHR::Tracer tracer{"layer item findAllChildItems recursive", category()};

    QList<QGraphicsItem*> itemList(item->childItems());

    for (int end = itemList.length(), i = 0; i < end; ++i)
        itemList += findAllChildItems(itemList.at(i));

    return itemList;
}

}
