// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "resizehandleitem.h"

#include "formeditortracing.h"

#include <QPainter>

namespace QmlDesigner {

using FormEditorTracing::category;

ResizeHandleItem::ResizeHandleItem(QGraphicsItem *parent, const ResizeController &resizeController)
    : QGraphicsItem(parent),
    m_weakResizeController(resizeController.toWeakResizeController())
{
    NanotraceHR::Tracer tracer{"resize handle item constructor", category()};

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    setAcceptedMouseButtons(Qt::NoButton);
}

ResizeHandleItem::~ResizeHandleItem() = default;

void ResizeHandleItem::setHandlePosition(const QPointF & globalPosition, const QPointF & itemSpacePosition)
{
    NanotraceHR::Tracer tracer{"resize handle item set handle position", category()};

    m_itemSpacePosition = itemSpacePosition;
    setPos(globalPosition);
}

QRectF ResizeHandleItem::boundingRect() const
{
    return {- 5., - 5., 9., 9.};
}

void ResizeHandleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /* option */, QWidget * /* widget */)
{
    NanotraceHR::Tracer tracer{"resize handle item paint", category()};

    painter->save();
    QPen pen = painter->pen();
    pen.setWidth(1);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setBrush(QColor(255, 255, 255));
    painter->drawRect(QRectF(-3., -3., 5., 5.));

    painter->restore();
}


ResizeController ResizeHandleItem::resizeController() const
{
    NanotraceHR::Tracer tracer{"resize handle item resize controller", category()};

    return ResizeController(m_weakResizeController.toResizeController());
}

ResizeHandleItem* ResizeHandleItem::fromGraphicsItem(QGraphicsItem *item)
{
    return qgraphicsitem_cast<ResizeHandleItem*>(item);
}

bool ResizeHandleItem::isTopLeftHandle() const
{
    NanotraceHR::Tracer tracer{"resize handle item is top left handle", category()};

    return resizeController().isTopLeftHandle(this);
}

bool ResizeHandleItem::isTopRightHandle() const
{
    NanotraceHR::Tracer tracer{"resize handle item is top right handle", category()};

    return resizeController().isTopRightHandle(this);
}

bool ResizeHandleItem::isBottomLeftHandle() const
{
    NanotraceHR::Tracer tracer{"resize handle item is bottom left handle", category()};

    return resizeController().isBottomLeftHandle(this);
}

bool ResizeHandleItem::isBottomRightHandle() const
{
    NanotraceHR::Tracer tracer{"resize handle item is bottom right handle", category()};

    return resizeController().isBottomRightHandle(this);
}

bool ResizeHandleItem::isTopHandle() const
{
    NanotraceHR::Tracer tracer{"resize handle item is top handle", category()};

    return resizeController().isTopHandle(this);
}

bool ResizeHandleItem::isLeftHandle() const
{
    NanotraceHR::Tracer tracer{"resize handle item is left handle", category()};

    return resizeController().isLeftHandle(this);
}

bool ResizeHandleItem::isRightHandle() const
{
    NanotraceHR::Tracer tracer{"resize handle item is right handle", category()};

    return resizeController().isRightHandle(this);
}

bool ResizeHandleItem::isBottomHandle() const
{
    NanotraceHR::Tracer tracer{"resize handle item is bottom handle", category()};

    return resizeController().isBottomHandle(this);
}

QPointF ResizeHandleItem::itemSpacePosition() const
{
    NanotraceHR::Tracer tracer{"resize handle item item space position", category()};

    return m_itemSpacePosition;
}
}
