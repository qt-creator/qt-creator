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

#include "keyframeitem.h"
#include "handleitem.h"

#include <QPainter>

#include <cmath>

namespace DesignTools {

KeyframeItem::KeyframeItem(QGraphicsItem *parent)
    : SelectableItem(parent)
    , m_frame()
{}

KeyframeItem::KeyframeItem(const Keyframe &keyframe, QGraphicsItem *parent)
    : SelectableItem(parent)
    , m_transform()
    , m_frame(keyframe)
    , m_left(keyframe.hasLeftHandle() ? new HandleItem(this) : nullptr)
    , m_right(keyframe.hasRightHandle() ? new HandleItem(this) : nullptr)
{
    auto updatePosition = [this]() { this->updatePosition(true); };
    connect(this, &QGraphicsObject::xChanged, updatePosition);
    connect(this, &QGraphicsObject::yChanged, updatePosition);

    if (m_left) {
        m_left->setPos(m_frame.leftHandle() - m_frame.position());
        auto updateLeftHandle = [this]() { updateHandle(m_left); };
        connect(m_left, &QGraphicsObject::xChanged, updateLeftHandle);
        connect(m_left, &QGraphicsObject::yChanged, updateLeftHandle);
        m_left->hide();
    }

    if (m_right) {
        m_right->setPos(m_frame.rightHandle() - m_frame.position());
        auto updateRightHandle = [this]() { updateHandle(m_right); };
        connect(m_right, &QGraphicsObject::xChanged, updateRightHandle);
        connect(m_right, &QGraphicsObject::yChanged, updateRightHandle);
        m_right->hide();
    }

    setPos(m_frame.position());
}

int KeyframeItem::type() const
{
    return Type;
}

QRectF KeyframeItem::boundingRect() const
{
    QPointF topLeft(-m_style.size / 2.0, -m_style.size / 2.0);
    return QRectF(topLeft, -topLeft);
}

void KeyframeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QPen pen = painter->pen();
    pen.setColor(Qt::black);

    painter->save();
    painter->setPen(pen);
    painter->setBrush(selected() ? Qt::red : m_style.color);
    painter->drawEllipse(boundingRect());

    painter->restore();
}

KeyframeItem::~KeyframeItem() {}

Keyframe KeyframeItem::keyframe() const
{
    return m_frame;
}

void KeyframeItem::setComponentTransform(const QTransform &transform)
{
    m_transform = transform;

    if (m_left)
        m_left->setPos(m_transform.map(m_frame.leftHandle() - m_frame.position()));

    if (m_right)
        m_right->setPos(m_transform.map(m_frame.rightHandle() - m_frame.position()));

    setPos(m_transform.map(m_frame.position()));
}

void KeyframeItem::setStyle(const CurveEditorStyle &style)
{
    m_style = style.keyframeStyle;

    if (m_left)
        m_left->setStyle(style);

    if (m_right)
        m_right->setStyle(style);
}

void KeyframeItem::updatePosition(bool update)
{
    bool ok = false;
    QPointF position = m_transform.inverted(&ok).map(pos());

    if (!ok)
        return;

    QPointF oldPosition = m_frame.position();
    m_frame.setPosition(position);

    if (m_left)
        updateHandle(m_left, false);

    if (m_right)
        updateHandle(m_right, false);

    if (update) {
        emit redrawCurve();
        emit keyframeMoved(this, position - oldPosition);
    }
}

void KeyframeItem::moveKeyframe(const QPointF &direction)
{
    this->blockSignals(true);
    setPos(m_transform.map(m_frame.position() + direction));
    updatePosition(false);
    this->blockSignals(false);
    emit redrawCurve();
}

void KeyframeItem::moveHandle(HandleSlot handle, double deltaAngle, double deltaLength)
{
    auto move = [this, deltaAngle, deltaLength](HandleItem *item) {
        QLineF current(QPointF(0.0, 0.0), item->pos());
        current.setAngle(current.angle() + deltaAngle);
        current.setLength(current.length() + deltaLength);
        item->setPos(current.p2());
        updateHandle(item, false);
    };

    this->blockSignals(true);

    if (handle == HandleSlot::Left)
        move(m_left);
    else if (handle == HandleSlot::Right)
        move(m_right);

    this->blockSignals(false);

    emit redrawCurve();
}

void KeyframeItem::updateHandle(HandleItem *handle, bool emitChanged)
{
    bool ok = false;

    QPointF handlePosition = m_transform.inverted(&ok).map(handle->pos());

    if (!ok)
        return;

    QPointF oldPosition;
    QPointF newPosition;
    HandleSlot slot = HandleSlot::Undefined;
    if (handle == m_left) {
        slot = HandleSlot::Left;
        oldPosition = m_frame.leftHandle();
        m_frame.setLeftHandle(m_frame.position() + handlePosition);
        newPosition = m_frame.leftHandle();
    } else {
        slot = HandleSlot::Right;
        oldPosition = m_frame.rightHandle();
        m_frame.setRightHandle(m_frame.position() + handlePosition);
        newPosition = m_frame.rightHandle();
    }

    if (emitChanged) {
        QLineF oldLine(m_frame.position(), oldPosition);
        QLineF newLine(m_frame.position(), newPosition);

        QLineF mappedOld = m_transform.map(oldLine);
        QLineF mappedNew = m_transform.map(newLine);

        auto angle = mappedOld.angleTo(mappedNew);
        auto deltaLength = mappedNew.length() - mappedOld.length();

        emit redrawCurve();
        emit handleMoved(this, slot, angle, deltaLength);
    }
}

QVariant KeyframeItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange) {
        bool ok;
        QPointF position = m_transform.inverted(&ok).map(value.toPointF());
        if (ok) {
            position.setX(std::round(position.x()));
            return QVariant(m_transform.map(position));
        }
    }

    return QGraphicsItem::itemChange(change, value);
}

void KeyframeItem::selectionCallback()
{
    auto setHandleVisibility = [](HandleItem *handle, bool visible) {
        if (handle)
            handle->setVisible(visible);
    };

    if (selected()) {
        setHandleVisibility(m_left, true);
        setHandleVisibility(m_right, true);
    } else {
        setHandleVisibility(m_left, false);
        setHandleVisibility(m_right, false);
    }
}

} // End namespace DesignTools.
