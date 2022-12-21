// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mytypes.h"
#include <QGraphicsObject>

namespace ScxmlEditor {

namespace PluginInterface {

class LayoutItem : public QGraphicsObject
{
public:
    LayoutItem(const QRectF &br, QGraphicsItem *parent = nullptr);

    int type() const override
    {
        return LayoutType;
    }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void setBoundingRect(const QRectF &r);

private:
    QRectF m_boundingRect;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
