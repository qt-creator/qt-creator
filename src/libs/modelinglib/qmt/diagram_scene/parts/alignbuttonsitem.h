// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/diagram_scene/capabilities/alignable.h"

#include <QGraphicsItem>

namespace qmt {

class IAlignable;

class AlignButtonsItem : public QGraphicsItem
{
    class AlignButtonItem;

public:
    enum {
        NormalPixmapWidth = 14,
        NormalPixmapHeight = NormalPixmapWidth,
        InnerBorder = 2,
        NormalButtonWidth = NormalPixmapWidth + 2 * InnerBorder,
        NormalButtonHeight = NormalPixmapHeight + 2 * InnerBorder,
        HorizontalDistanceToObject = 4,
        VerticalDistanceToObejct = HorizontalDistanceToObject
    };

    explicit AlignButtonsItem(IAlignable *alignable, QGraphicsItem *parent = nullptr);
    ~AlignButtonsItem() override;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void clear();
    void addButton(IAlignable::AlignType alignType, const QString &identifier, qreal pos);

private:
    IAlignable *m_alignable = nullptr;
    QList<AlignButtonItem *> m_alignItems;
};

} // namespace qmt
