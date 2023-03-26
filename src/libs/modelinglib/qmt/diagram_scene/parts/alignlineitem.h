// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsItem>

namespace qmt {

class AlignLineItem : public QGraphicsItem
{
public:
    enum Direction {
        Horizontal,
        Vertical
    };

    explicit AlignLineItem(Direction direction, QGraphicsItem *parent = nullptr);
    ~AlignLineItem() override;

    void setLine(qreal pos);
    void setLine(qreal pos, qreal otherPos1, qreal otherPos2);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

    Direction m_direction = Horizontal;
    QGraphicsLineItem *m_alignLine = nullptr;
    QGraphicsLineItem *m_highlightLine = nullptr;
};

} // namespace qmt
