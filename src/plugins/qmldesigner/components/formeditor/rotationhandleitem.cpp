// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rotationhandleitem.h"
#include "formeditortracing.h"

#include <QPainter>
#include <QDebug>

namespace QmlDesigner {

using FormEditorTracing::category;

RotationHandleItem::RotationHandleItem(QGraphicsItem *parent, const RotationController &rotationController)
    : QGraphicsItem(parent)
    , m_weakRotationController(rotationController.toWeakRotationController())
{
    NanotraceHR::Tracer tracer{"rotation handle item constructor", category()};

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    setAcceptedMouseButtons(Qt::NoButton);
}

RotationHandleItem::~RotationHandleItem() = default;

void RotationHandleItem::setHandlePosition(const QPointF & globalPosition, const QPointF & itemSpacePosition, const qreal rotation)
{
    NanotraceHR::Tracer tracer{"rotation handle item constructor", category()};

    m_itemSpacePosition = itemSpacePosition;
    setRotation(rotation);
    setPos(globalPosition);
}

QRectF RotationHandleItem::boundingRect() const
{
    NanotraceHR::Tracer tracer{"rotation handle item bounding rect", category()};

    QRectF rectangle;

    if (isTopLeftHandle()) {
        rectangle = { -30., -30., 27., 27.};
    }
    else if (isTopRightHandle()) {
        rectangle = { 3., -30., 27., 27.};
    }
    else if (isBottomLeftHandle()) {
        rectangle = { -30., 3., 27., 27.};
    }
    else if (isBottomRightHandle()) {
        rectangle = { 3., 3., 27., 27.};
    }
    return rectangle;
}

void RotationHandleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /* option */, QWidget * /* widget */)
{
    NanotraceHR::Tracer tracer{"rotation handle item paint", category()};

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


RotationController RotationHandleItem::rotationController() const
{
    NanotraceHR::Tracer tracer{"rotation handle item rotation controller", category()};

    return RotationController(m_weakRotationController.toRotationController());
}

RotationHandleItem* RotationHandleItem::fromGraphicsItem(QGraphicsItem *item)
{
    return qgraphicsitem_cast<RotationHandleItem*>(item);
}

bool RotationHandleItem::isTopLeftHandle() const
{
    NanotraceHR::Tracer tracer{"rotation handle item is top left handle", category()};

    return rotationController().isTopLeftHandle(this);
}

bool RotationHandleItem::isTopRightHandle() const
{
    NanotraceHR::Tracer tracer{"rotation handle item is top right handle", category()};

    return rotationController().isTopRightHandle(this);
}

bool RotationHandleItem::isBottomLeftHandle() const
{
    NanotraceHR::Tracer tracer{"rotation handle item is bottom left handle", category()};

    return rotationController().isBottomLeftHandle(this);
}

bool RotationHandleItem::isBottomRightHandle() const
{
    NanotraceHR::Tracer tracer{"rotation handle item is bottom right handle", category()};

    return rotationController().isBottomRightHandle(this);
}

QPointF RotationHandleItem::itemSpacePosition() const
{
    NanotraceHR::Tracer tracer{"rotation handle item item space position", category()};

    return m_itemSpacePosition;
}
}
