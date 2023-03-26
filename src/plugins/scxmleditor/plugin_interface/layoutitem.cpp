// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "layoutitem.h"

using namespace ScxmlEditor::PluginInterface;

LayoutItem::LayoutItem(const QRectF &br, QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_boundingRect(br)
{
    setZValue(-100);
}

QRectF LayoutItem::boundingRect() const
{
    return m_boundingRect;
}

void LayoutItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter)
    Q_UNUSED(option)
    Q_UNUSED(widget)
}

void LayoutItem::setBoundingRect(const QRectF &r)
{
    prepareGeometryChange();
    m_boundingRect = r;
}
