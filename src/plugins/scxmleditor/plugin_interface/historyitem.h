// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "connectableitem.h"

namespace ScxmlEditor {

namespace PluginInterface {

/**
 * @brief The HistoryItem class represents History-state of the SCXML-standard. It is an extended class from the ConnectableItem.
 */
class HistoryItem : public ConnectableItem
{
    Q_OBJECT

public:
    HistoryItem(const QPointF &pos = QPointF(), BaseItem *parent = nullptr);

    int type() const override
    {
        return HistoryType;
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
