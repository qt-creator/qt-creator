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

#include "handleitem.h"
#include "graphicsscene.h"
#include "keyframeitem.h"
#include "utils.h"

#include <QPainter>

namespace DesignTools {

struct HandleGeometry
{
    HandleGeometry(const QPointF &pos, const HandleItemStyleOption &style)
    {
        QPointF topLeft(-style.size / 2.0, -style.size / 2.0);
        handle = QRectF(topLeft, -topLeft);
        toKeyframe = QLineF(QPointF(0.0, 0.0), -pos);
        angle = -toKeyframe.angle() + 45.0;
    }

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
    return geom.handle;
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

void HandleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if (locked())
        return;

    Q_UNUSED(option)
    Q_UNUSED(widget)

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
        if (keyframe()) {
            QPointF pos = value.toPointF();
            if (m_slot == HandleItem::Slot::Left) {
                if (pos.x() > 0.0)
                    pos.rx() = 0.0;

            } else if (m_slot == HandleItem::Slot::Right) {
                if (pos.x() < 0.0)
                    pos.rx() = 0.0;
            }
            return QVariant(pos);
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

} // End namespace DesignTools.
