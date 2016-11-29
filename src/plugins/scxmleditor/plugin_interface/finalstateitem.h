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

#include "connectableitem.h"

namespace ScxmlEditor {

namespace PluginInterface {

/**
 * @brief The FinalStateItem class represents Final-state of the SCXML-standard. It is a extended class from the ConnectableItem.
 */
class FinalStateItem : public ConnectableItem
{
    Q_OBJECT

public:
    FinalStateItem(const QPointF &pos = QPointF(), BaseItem *parent = nullptr);

    int type() const override
    {
        return FinalStateType;
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

protected:
    void updatePolygon() override;
    bool canStartTransition(ItemType type) const override;

private:
    qreal m_size = 1.0;
    QPen m_pen;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
