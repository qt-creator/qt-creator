/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include "curveeditorstyle.h"
#include "selectableitem.h"

namespace DesignTools {

class KeyframeItem;

class HandleItem : public SelectableItem
{
    Q_OBJECT

public:
    enum { Type = ItemTypeHandle };

    enum class Slot { Undefined, Left, Right };

    HandleItem(QGraphicsItem *parent, HandleItem::Slot slot);

    ~HandleItem() override;

    int type() const override;

    QRectF boundingRect() const override;

    bool contains(const QPointF &point) const override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void underMouseCallback() override;

    bool keyframeSelected() const;

    KeyframeItem *keyframe() const;

    Slot slot() const;

    void setStyle(const CurveEditorStyle &style);

protected:
    QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) override;

private:
    Slot m_slot;

    HandleItemStyleOption m_style;
};

} // End namespace DesignTools.
