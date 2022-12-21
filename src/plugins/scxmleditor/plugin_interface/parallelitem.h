// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "stateitem.h"

namespace ScxmlEditor {

namespace PluginInterface {

/**
 * @brief The ParalllelItem class represents Parallel-state of the SCXML-standard. It is a extended class from the StateItem.
 */
class ParallelItem final : public StateItem
{
public:
    explicit ParallelItem(const QPointF &pos = QPointF(), BaseItem *parent = nullptr);

    int type() const override
    {
        return ParallelType;
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void doLayout(int d) override;

protected:
    void updatePolygon() override;

private:
    QPixmap m_pixmap;
    QRect m_pixmapRect;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
