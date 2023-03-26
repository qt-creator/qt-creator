// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "curveeditorstyle.h"
#include "selectableitem.h"

namespace QmlDesigner {

class KeyframeItem;
class CurveSegment;

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

    CurveSegment segment() const;

    KeyframeItem *keyframe() const;

    Slot slot() const;

    void setStyle(const CurveEditorStyle &style);

protected:
    QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) override;

private:
    Slot m_slot;

    HandleItemStyleOption m_style;

    QPointF m_validPos;
};

} // End namespace QmlDesigner.
