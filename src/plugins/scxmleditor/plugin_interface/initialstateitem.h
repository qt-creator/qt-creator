// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
