// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "keyframeitem.h"
#include "curveitem.h"
#include "handleitem.h"

#include <QApplication>
#include <QPainter>
#include <qnamespace.h>
#include <QGraphicsSceneMouseEvent>

#include <cmath>

namespace QmlDesigner {

KeyframeItem::KeyframeItem(QGraphicsItem *parent)
    : SelectableItem(parent)
    , m_transform()
    , m_style()
    , m_frame()
    , m_left(nullptr)
    , m_right(nullptr)
    , m_validPos()
{}

KeyframeItem::KeyframeItem(const Keyframe &keyframe, QGraphicsItem *parent)
    : SelectableItem(parent)
    , m_transform()
    , m_style()
    , m_frame()
    , m_left(nullptr)
    , m_right(nullptr)
    , m_validPos()
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

void KeyframeItem::paint(QPainter *painter,
                         [[maybe_unused]] const QStyleOptionGraphicsItem *option,
                         [[maybe_unused]] QWidget *widget)
{
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

Keyframe KeyframeItem::keyframe(bool remap) const
{
    if (remap) {
        auto frame = m_frame;
        auto pos = frame.position();
        auto center = m_min + ((m_max - m_min) / 2.0);
        pos.ry() = pos.y() > center ? 1.0 : 0.0;
        frame.setPosition(pos);
        return frame;
    }
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

CurveSegment KeyframeItem::segment(HandleItem::Slot slot) const
{
    if (auto *curveItem = qgraphicsitem_cast<CurveItem *>(parentItem()))
        return curveItem->segment(this, slot);

    return CurveSegment();
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

void KeyframeItem::remapValue(double min, double max)
{
    auto center = m_min + ((m_max - m_min) / 2.0);
    auto pos = m_frame.position();
    pos.ry() = pos.y() > center ? max : min;
    m_frame.setPosition(pos);

    m_max = max;
    m_min = min;
    setKeyframe(m_frame);
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
        if (!scene())
            return QGraphicsItem::itemChange(change, value);

        CurveItem *curveItem = qgraphicsitem_cast<CurveItem *>(parentItem());
        if (!curveItem)
            return QGraphicsItem::itemChange(change, value);

        CurveSegment lseg = segment(HandleItem::Slot::Left);
        CurveSegment rseg = segment(HandleItem::Slot::Right);

        auto legalLeft = [this, curveItem, &lseg]() {
            if (curveItem->isFirst(this))
                return true;
            else
                return lseg.isLegal();
        };

        auto legalRight = [this, curveItem, &rseg]() {
            if (curveItem->isLast(this))
                return true;
            else
                return rseg.isLegal();
        };

        bool ok;
        QPointF position = m_transform.inverted(&ok).map(value.toPointF());
        if (ok) {
            position.setX(std::round(position.x()));

            if (curveItem->valueType() == PropertyTreeItem::ValueType::Integer)
                position.setY(std::round(position.y()));
            else if (curveItem->valueType() == PropertyTreeItem::ValueType::Bool) {
                double center = m_min + ((m_max - m_min) / 2.0);
                position.setY(position.y() > center ? m_max : m_min);
            }

            if (!legalLeft() || !legalRight()) {
                return QVariant(m_transform.map(position));
            } else {
                lseg.moveRightTo(position);
                rseg.moveLeftTo(position);

                if (legalLeft() && legalRight()) {
                    if (qApp->keyboardModifiers().testFlag(Qt::ShiftModifier) && m_validPos.has_value()) {
                        if (m_firstPos) {
                            auto firstToNow = QLineF(*m_firstPos, position);
                            if (std::abs(firstToNow.dx()) > std::abs(firstToNow.dy()))
                                m_validPos = QPointF(position.x(), m_firstPos->y());
                            else
                                m_validPos = QPointF(m_firstPos->x(), position.y());
                        }

                    } else {
                        m_validPos = position;
                    }
                }

                return QVariant(m_transform.map(*m_validPos));
            }
        }
    }

    return QGraphicsItem::itemChange(change, value);
}

void KeyframeItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    bool ok;
    m_firstPos = m_transform.inverted(&ok).map(event->scenePos());
    if (!ok)
        m_firstPos = std::nullopt;

    SelectableItem::mousePressEvent(event);
    if (auto *curveItem = qgraphicsitem_cast<CurveItem *>(parentItem()))
        curveItem->setHandleVisibility(false);
}

void KeyframeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    m_firstPos = std::nullopt;
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

} // End namespace QmlDesigner.
