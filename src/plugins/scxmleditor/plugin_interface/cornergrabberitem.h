// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "baseitem.h"

QT_FORWARD_DECLARE_CLASS(QGraphicsSceneHoverEvent)
QT_FORWARD_DECLARE_CLASS(QGraphicsSceneMouseEvent)

namespace ScxmlEditor {

namespace PluginInterface {

/**
 * @brief The CornerGrabberItem class provides the way to connect/create item(s) straight from the second item.
 *
 */
class CornerGrabberItem : public QGraphicsObject
{
public:
    explicit CornerGrabberItem(QGraphicsItem *parent, Qt::CursorShape cshape = Qt::PointingHandCursor);

    enum GrabberType {
        Square,
        Circle
    };

    /**
     * @brief type of the item
     */
    int type() const override
    {
        return CornerGrabberType;
    }

    QRectF boundingRect() const override
    {
        return m_rect;
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QPointF pressedPoint()
    {
        return m_pressPoint;
    }

    void setGrabberType(GrabberType type)
    {
        m_grabberType = type;
    }

    void setSelected(bool para);
    bool isSelected() const
    {
        return m_selected;
    }

private:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *e) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *e) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *e) override;
    void updateCursor();

    QRectF m_rect, m_drawingRect;
    QPointF m_pressPoint;
    GrabberType m_grabberType = Square;
    Qt::CursorShape m_cursorShape;
    bool m_selected = false;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
