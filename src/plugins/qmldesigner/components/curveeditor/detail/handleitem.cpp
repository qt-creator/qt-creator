// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "handleitem.h"
#include "curvesegment.h"
#include "graphicsscene.h"
#include "keyframeitem.h"
#include "curveeditorutils.h"

#include <QPainter>

namespace QmlDesigner {

struct HandleGeometry
{
    HandleGeometry(const QPointF &pos, const HandleItemStyleOption &style)
    {
        QPointF topLeft(-style.size / 2.0, -style.size / 2.0);
        handle = QRectF(topLeft, -topLeft);
        toKeyframe = QLineF(QPointF(0.0, 0.0), -pos);
        angle = -toKeyframe.angle() + 45.0;
        bbox = handle.united(QRectF(-pos, QSizeF(1.0,1.0)));
    }

    QRectF bbox;

    QRectF handle;

    QLineF toKeyframe;

    double angle;
};

HandleItem::HandleItem(QGraphicsItem *parent, HandleItem::Slot slot)
    : SelectableItem(parent)
    , m_slot(slot)
    , m_style()
{
    setFlag(QGraphicsItem::ItemStacksBehindParent, true);
}

HandleItem::~HandleItem() {}

int HandleItem::type() const
{
    return Type;
}

bool HandleItem::keyframeSelected() const
{
    if (auto *frame = keyframe())
        return frame->selected();

    return false;
}

CurveSegment HandleItem::segment() const
{
    if (KeyframeItem *parent = qgraphicsitem_cast<KeyframeItem *>(parentItem()))
        return parent->segment(m_slot);

    return CurveSegment();
}

KeyframeItem *HandleItem::keyframe() const
{
    if (KeyframeItem *parent = qgraphicsitem_cast<KeyframeItem *>(parentItem()))
        return parent;

    return nullptr;
}

HandleItem::Slot HandleItem::slot() const
{
    return m_slot;
}

QRectF HandleItem::boundingRect() const
{
    HandleGeometry geom(pos(), m_style);
    return geom.bbox;
}

bool HandleItem::contains(const QPointF &point) const
{
    if (KeyframeItem *parent = keyframe()) {
        HandleGeometry geom(pos(), m_style);
        geom.handle.moveCenter(parent->pos() + pos());
        return geom.handle.contains(point);
    }
    return false;
}

void HandleItem::underMouseCallback()
{
    if (auto *gscene = qobject_cast<GraphicsScene *>(scene()))
        gscene->handleUnderMouse(this);
}

void HandleItem::paint(QPainter *painter,
                       [[maybe_unused]] const QStyleOptionGraphicsItem *option,
                       [[maybe_unused]] QWidget *widget)
{
    if (locked())
        return;

    QColor handleColor(selected() ? m_style.selectionColor : m_style.color);

    if (activated())
        handleColor = m_style.activeColor;
    if (isUnderMouse())
        handleColor = m_style.hoverColor;

    HandleGeometry geom(pos(), m_style);

    QPen pen = painter->pen();
    pen.setWidthF(m_style.lineWidth);
    pen.setColor(handleColor);

    painter->save();
    painter->setPen(pen);

    painter->drawLine(geom.toKeyframe);
    painter->rotate(geom.angle);
    painter->fillRect(geom.handle, handleColor);

    painter->restore();
}

void HandleItem::setStyle(const CurveEditorStyle &style)
{
    m_style = style.handleStyle;
}

QVariant HandleItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange) {

        if (!scene())
            return QGraphicsItem::itemChange(change, value);

        if (KeyframeItem *keyItem = keyframe()) {
            CurveSegment seg = segment();
            if (!seg.isLegal())
                return value;

            QPointF pos = value.toPointF();
            QPointF relativePosition = keyItem->transform().inverted().map(pos);

            if (m_slot == HandleItem::Slot::Left) {
                if (pos.x() > 0.0)
                    pos.rx() = 0.0;

                Keyframe key = seg.right();
                key.setLeftHandle(key.position() + relativePosition);
                seg.setRight(key);

            } else if (m_slot == HandleItem::Slot::Right) {
                if (pos.x() < 0.0)
                    pos.rx() = 0.0;

                Keyframe key = seg.left();
                key.setRightHandle(key.position() + relativePosition);
                seg.setLeft(key);
            }

            if (seg.isLegal())
                m_validPos = pos;

            return QVariant(m_validPos);
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

} // End namespace QmlDesigner.
