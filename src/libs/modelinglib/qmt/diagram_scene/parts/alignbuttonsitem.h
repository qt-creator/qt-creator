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
