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

class InitialWarningItem;

/**
 * @brief The InitialStateItem class represents Initial-state of the SCXML-standard. It is a extended class from the ConnectableItem.
 */
class InitialStateItem : public ConnectableItem
{
public:
    explicit InitialStateItem(const QPointF &pos = QPointF(), BaseItem *parent = nullptr);

    int type() const override
    {
        return InitialStateType;
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    InitialWarningItem *warningItem() const;

    void checkWarnings() override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void updatePolygon() override;
    bool canStartTransition(ItemType type) const override;

private:
    void checkWarningItems();
    InitialWarningItem *m_warningItem = nullptr;
    qreal m_size = 1.0;
    QPen m_pen;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
