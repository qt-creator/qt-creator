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
