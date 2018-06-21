/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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
