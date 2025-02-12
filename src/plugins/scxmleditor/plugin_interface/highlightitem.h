// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "baseitem.h"

#include <QBrush>
#include <QGraphicsObject>
#include <QPen>
#include <QPointer>

namespace ScxmlEditor {

namespace PluginInterface {

class HighlightItem : public QGraphicsObject
{
public:
    HighlightItem(BaseItem *parent = nullptr);

    int type() const override
    {
        return HighlightType;
    }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void advance(int phase) override;

private:
    QPointer<BaseItem> m_baseItem;
    QRectF m_boundingRect;
    QBrush m_brush;
    QPen m_pen;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
