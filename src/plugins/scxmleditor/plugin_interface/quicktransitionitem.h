// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "baseitem.h"

#include <QBrush>
#include <QPen>

namespace ScxmlEditor {

namespace PluginInterface {

class QuickTransitionItem : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit QuickTransitionItem(int index = 0, ItemType connectionType = UnknownType, QGraphicsItem *parent = nullptr);

    /**
     * @brief type of the item
     */
    int type() const override
    {
        return QuickTransitionType;
    }

    ItemType connectionType() const
    {
        return m_connectionType;
    }

    void setConnectionType(ItemType type);

    QRectF boundingRect() const override
    {
        return m_rect;
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *e) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *e) override;

    ItemType m_connectionType;
    QRectF m_rect;
    QBrush m_brush;
    QRectF m_drawingRect;
    QRectF m_stateRect;
    QPen m_pen;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
