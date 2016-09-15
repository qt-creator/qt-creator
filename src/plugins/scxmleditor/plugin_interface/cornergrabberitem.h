/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
