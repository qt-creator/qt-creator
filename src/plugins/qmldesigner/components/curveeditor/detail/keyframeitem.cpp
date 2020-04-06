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
#include "curveitem.h"
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
    , m_frame()
    , m_left(nullptr)
    , m_right(nullptr)
{
    setKeyframe(keyframe);
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
    Q_UNUSED(option)
    Q_UNUSED(widget)

    QColor mainColor = selected() ? m_style.selectionColor : m_style.color;
    QColor borderColor = isUnified() ? m_style.unifiedColor : m_style.splitColor;

    if (locked()) {
        mainColor = m_style.lockedColor;
        borderColor = m_style.lockedColor;
    }

    QPen pen = painter->pen();
    pen.setWidthF(1.);
    pen.setColor(borderColor);

    painter->save();
    painter->setPen(pen);
    painter->setBrush(mainColor);
    painter->drawEllipse(boundingRect());
    painter->restore();
}

void KeyframeItem::lockedCallback()
{
    SelectableItem::lockedCallback();

    if (m_left)
        m_left->setLocked(locked());

    if (m_right)
        m_right->setLocked(locked());
}

KeyframeItem::~KeyframeItem() {}

Keyframe KeyframeItem::keyframe() const
{
    return m_frame;
}

bool KeyframeItem::isUnified() const
{
    return m_frame.isUnified();
}

bool KeyframeItem::hasLeftHandle() const
{
    return m_frame.hasLeftHandle();
}

bool KeyframeItem::hasRightHandle() const
{
    return m_frame.hasRightHandle();
}

bool KeyframeItem::hasActiveHandle() const
{
    if (m_left && m_left->activated())
        return true;

    if (m_right && m_right->activated())
        return true;

    return false;
}

HandleItem *KeyframeItem::leftHandle() const
{
    return m_left;
}

HandleItem *KeyframeItem::rightHandle() const
{
    return m_right;
}

QTransform KeyframeItem::transform() const
{
    return m_transform;
}

void KeyframeItem::setHandleVisibility(bool visible)
{
    m_visibleOverride = visible;

    if (visible) {
        if (m_left)
            m_left->show();
        if (m_right)
            m_right->show();
    } else {
        if (m_left)
            m_left->hide();
        if (m_right)
            m_right->hide();
    }
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

void KeyframeItem::setKeyframe(const Keyframe &keyframe)
{
    bool needsConnection = m_frame.position().isNull();

    m_frame = keyframe;

    if (needsConnection) {
        auto updatePosition = [this]() { this->updatePosition(true); };
        connect(this, &QGraphicsObject::xChanged, updatePosition);
        connect(this, &QGraphicsObject::yChanged, updatePosition);
    }

    if (m_frame.hasLeftHandle()) {
        if (!m_left) {
            m_left = new HandleItem(this, HandleItem::Slot::Left);
            auto updateLeftHandle = [this]() { updateHandle(m_left); };
            connect(m_left, &QGraphicsObject::xChanged, updateLeftHandle);
            connect(m_left, &QGraphicsObject::yChanged, updateLeftHandle);
        }
        m_left->setPos(m_transform.map(m_frame.leftHandle() - m_frame.position()));
    } else if (m_left) {
        delete m_left;
        m_left = nullptr;
    }

    if (m_frame.hasRightHandle()) {
        if (!m_right) {
            m_right = new HandleItem(this, HandleItem::Slot::Right);
            auto updateRightHandle = [this]() { updateHandle(m_right); };
            connect(m_right, &QGraphicsObject::xChanged, updateRightHandle);
            connect(m_right, &QGraphicsObject::yChanged, updateRightHandle);
        }
        m_right->setPos(m_transform.map(m_frame.rightHandle() - m_frame.position()));
    } else if (m_right) {
        delete m_right;
        m_right = nullptr;
    }

    setPos(m_transform.map(m_frame.position()));
}

void KeyframeItem::toggleUnified()
{
    if (!m_left || !m_right)
        return;

    if (m_frame.isUnified())
        m_frame.setUnified(false);
    else
        m_frame.setUnified(true);
}

void KeyframeItem::setActivated(bool active, HandleItem::Slot slot)
{
    if (isUnified() && m_left && m_right) {
        m_left->setActivated(active);
        m_right->setActivated(active);
        return;
    }

    if (slot == HandleItem::Slot::Left && m_left)
        m_left->setActivated(active);
    else if (slot == HandleItem::Slot::Right && m_right)
        m_right->setActivated(active);
}

void KeyframeItem::setInterpolation(Keyframe::Interpolation interpolation)
{
    m_frame.setInterpolation(interpolation);
}

void KeyframeItem::setLeftHandle(const QPointF &pos)
{
    m_frame.setLeftHandle(pos);
    setKeyframe(m_frame);
}

void KeyframeItem::setRightHandle(const QPointF &pos)
{
    m_frame.setRightHandle(pos);
    setKeyframe(m_frame);
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

    if (update && position != oldPosition) {
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

void KeyframeItem::moveHandle(HandleItem::Slot slot, double deltaAngle, double deltaLength)
{
    auto move = [this, deltaAngle, deltaLength](HandleItem *item) {
        if (!item)
            return;

        QLineF current(QPointF(0.0, 0.0), item->pos());
        current.setAngle(current.angle() + deltaAngle);
        current.setLength(current.length() + deltaLength);
        item->setPos(current.p2());
        updateHandle(item, false);
    };

    this->blockSignals(true);

    if (slot == HandleItem::Slot::Left)
        move(m_left);
    else if (slot == HandleItem::Slot::Right)
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
    HandleItem::Slot slot = handle->slot();

    if (slot == HandleItem::Slot::Left) {
        oldPosition = m_frame.leftHandle();
        m_frame.setLeftHandle(m_frame.position() + handlePosition);
        newPosition = m_frame.leftHandle();
    } else if (slot == HandleItem::Slot::Right) {
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

            if (auto *curveItem = qgraphicsitem_cast<CurveItem *>(parentItem())) {
                if (curveItem->valueType() == ValueType::Integer)
                    position.setY(std::round(position.y()));
                else if (curveItem->valueType() == ValueType::Bool)
                    position.setY(position.y() > 0.5 ? 1.0 : 0.0);
            }

            return QVariant(m_transform.map(position));
        }
    }

    return QGraphicsItem::itemChange(change, value);
}

void KeyframeItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    SelectableItem::mousePressEvent(event);
    if (auto *curveItem = qgraphicsitem_cast<CurveItem *>(parentItem()))
        curveItem->setHandleVisibility(false);
}

void KeyframeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    SelectableItem::mouseReleaseEvent(event);
    if (auto *curveItem = qgraphicsitem_cast<CurveItem *>(parentItem()))
        curveItem->setHandleVisibility(true);
}

void KeyframeItem::selectionCallback()
{
    if (selected()) {
        if (m_visibleOverride) {
            setHandleVisibility(true);
        }
    } else {
        if (!m_visibleOverride) {
            setHandleVisibility(false);
        }
    }

    if (m_left)
        m_left->setSelected(selected());

    if (m_right)
        m_right->setSelected(selected());
}

} // End namespace DesignTools.
