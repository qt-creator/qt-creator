// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
