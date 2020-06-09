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
#include "curveitem.h"
#include "animationcurve.h"
#include "graphicsscene.h"
#include "keyframeitem.h"
#include "utils.h"

#include <QEasingCurve>
#include <QPainter>

#include <algorithm>
#include <cmath>
#include <sstream>

namespace DesignTools {

CurveItem::CurveItem(QGraphicsItem *parent)
    : CurveEditorItem(parent)
    , m_id(0)
    , m_style()
    , m_type(ValueType::Undefined)
    , m_component(PropertyTreeItem::Component::Generic)
    , m_transform()
    , m_keyframes()
    , m_itemDirty(false)
{}

CurveItem::CurveItem(unsigned int id, const AnimationCurve &curve, QGraphicsItem *parent)
    : CurveEditorItem(parent)
    , m_id(id)
    , m_style()
    , m_type(ValueType::Undefined)
    , m_component(PropertyTreeItem::Component::Generic)
    , m_transform()
    , m_keyframes()
    , m_itemDirty(false)
{
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setCurve(curve);
}

CurveItem::~CurveItem() {}

int CurveItem::type() const
{
    return Type;
}

QRectF CurveItem::boundingRect() const
{
    if (m_keyframes.empty())
        return QRectF();

    auto bbox = [](QRectF &bounds, const Keyframe &frame) {
        grow(bounds, frame.position());
        if (frame.hasLeftHandle())
            grow(bounds, frame.leftHandle());
        if (frame.hasRightHandle())
            grow(bounds, frame.rightHandle());
    };

    QPointF init = m_keyframes[0]->keyframe().position();
    QRectF bounds(init, init);
    for (auto *item : m_keyframes)
        bbox(bounds, item->keyframe());

    return m_transform.mapRect(bounds);
}

bool CurveItem::contains(const QPointF &point) const
{
    bool valid = false;
    QPointF transformed(m_transform.inverted(&valid).map(point));

    double widthX = std::abs(10.0 / scaleX(m_transform));
    double widthY = std::abs(10.0 / scaleY(m_transform));

    if (valid)
        return curve().intersects(transformed, widthX, widthY);

    return false;
}

void CurveItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (m_keyframes.size() > 1) {
        QPen pen = painter->pen();
        pen.setWidthF(m_style.width);

        painter->save();
        painter->setPen(pen);

        for (auto &&segment : curve().segments()) {
            if (segment.interpolation() == Keyframe::Interpolation::Easing) {
                pen.setColor(m_style.easingCurveColor);
            } else {
                if (locked())
                    pen.setColor(m_style.lockedColor);
                else if (isUnderMouse())
                    pen.setColor(m_style.hoverColor);
                else if (hasSelectedKeyframe())
                    pen.setColor(m_style.selectionColor);
                else
                    pen.setColor(m_style.color);
            }
            painter->setPen(pen);
            painter->drawPath(m_transform.map(segment.path()));
        }
        painter->restore();
    }
}

void CurveItem::lockedCallback()
{
    for (auto frame : m_keyframes)
        frame->setLocked(locked());

    setHandleVisibility(!locked());
}

bool CurveItem::isDirty() const
{
    return m_itemDirty;
}

bool CurveItem::hasActiveKeyframe() const
{
    for (auto *frame : m_keyframes) {
        if (frame->activated())
            return true;
    }
    return false;
}

bool CurveItem::hasActiveHandle() const
{
    for (auto *frame : m_keyframes) {
        if (frame->hasActiveHandle())
            return true;
    }
    return false;
}

bool CurveItem::hasSelectedKeyframe() const
{
    for (auto *frame : m_keyframes) {
        if (frame->selected())
            return true;
    }
    return false;
}

unsigned int CurveItem::id() const
{
    return m_id;
}

ValueType CurveItem::valueType() const
{
    return m_type;
}

PropertyTreeItem::Component CurveItem::component() const
{
    return m_component;
}

AnimationCurve CurveItem::curve() const
{
    std::vector<Keyframe> frames;
    frames.reserve(m_keyframes.size());
    for (auto *frameItem : m_keyframes)
        frames.push_back(frameItem->keyframe());

    return AnimationCurve(frames);
}

AnimationCurve CurveItem::resolvedCurve() const
{
    std::vector<AnimationCurve> tmp = curves();

    if (tmp.size() == 0)
        return AnimationCurve();

    if (tmp.size() == 1)
        return tmp[0];

    AnimationCurve out = tmp[0];
    for (size_t i = 1; i < tmp.size(); ++i)
        out.append(tmp[i]);

    return out;
}

std::vector<AnimationCurve> CurveItem::curves() const
{
    std::vector<AnimationCurve> out;

    std::vector<Keyframe> tmp;

    for (int i = 0; i < m_keyframes.size(); ++i) {
        KeyframeItem *item = m_keyframes[i];

        Keyframe current = item->keyframe();

        if (current.interpolation() == Keyframe::Interpolation::Easing && i > 0) {
            if (!tmp.empty()) {
                Keyframe previous = tmp.back();

                if (tmp.size() >= 2)
                    out.push_back(AnimationCurve(tmp));

                out.push_back(AnimationCurve(current.data().value<QEasingCurve>(),
                                             previous.position(),
                                             current.position()));

                tmp.clear();
                tmp.push_back(current);
            }
        } else {
            tmp.push_back(current);
        }
    }

    if (tmp.size() >= 2)
        out.push_back(AnimationCurve(tmp));

    return out;
}

QVector<KeyframeItem *> CurveItem::keyframes() const
{
    return m_keyframes;
}

QVector<KeyframeItem *> CurveItem::selectedKeyframes() const
{
    QVector<KeyframeItem *> out;
    for (auto *frame : m_keyframes) {
        if (frame->selected())
            out.push_back(frame);
    }
    return out;
}

QVector<HandleItem *> CurveItem::handles() const
{
    QVector<HandleItem *> out;
    for (auto *frame : m_keyframes) {
        if (auto *left = frame->leftHandle())
            out.push_back(left);
        if (auto *right = frame->rightHandle())
            out.push_back(right);
    }
    return out;
}

void CurveItem::restore()
{
    if (m_keyframes.empty())
        return;

    auto byTime = [](auto a, auto b) {
        return a->keyframe().position().x() < b->keyframe().position().x();
    };
    std::sort(m_keyframes.begin(), m_keyframes.end(), byTime);

    KeyframeItem *prevItem = m_keyframes[0];

    if (prevItem->hasLeftHandle())
        prevItem->setLeftHandle(QPointF());

    for (int i = 1; i < m_keyframes.size(); ++i) {
        KeyframeItem *currItem = m_keyframes[i];

        bool left = prevItem->hasRightHandle();
        bool right = currItem->hasLeftHandle();
        if (left != right) {
            if (left)
                prevItem->setRightHandle(QPointF());

            if (right)
                currItem->setLeftHandle(QPointF());
        }
        CurveSegment segment(prevItem->keyframe(), currItem->keyframe());
        currItem->setInterpolation(segment.interpolation());
        prevItem = currItem;
    }
}

void CurveItem::setDirty(bool dirty)
{
    m_itemDirty = dirty;
}

void CurveItem::setHandleVisibility(bool visible)
{
    for (auto frame : m_keyframes)
        frame->setHandleVisibility(visible);
}

void CurveItem::setValueType(ValueType type)
{
    m_type = type;
}

void CurveItem::setComponent(PropertyTreeItem::Component comp)
{
    m_component = comp;
}

void CurveItem::setCurve(const AnimationCurve &curve)
{
    freeClear(m_keyframes);

    for (const auto &frame : curve.keyframes()) {
        auto *item = new KeyframeItem(frame, this);
        item->setLocked(locked());
        item->setComponentTransform(m_transform);
        m_keyframes.push_back(item);
        QObject::connect(item, &KeyframeItem::redrawCurve, this, &CurveItem::emitCurveChanged);
        QObject::connect(item, &KeyframeItem::keyframeMoved, this, &CurveItem::keyframeMoved);
        QObject::connect(item, &KeyframeItem::handleMoved, this, &CurveItem::handleMoved);
    }

    emitCurveChanged();
}

QRectF CurveItem::setComponentTransform(const QTransform &transform)
{
    prepareGeometryChange();
    m_transform = transform;
    for (auto frame : m_keyframes)
        frame->setComponentTransform(transform);

    return boundingRect();
}

void CurveItem::setStyle(const CurveEditorStyle &style)
{
    m_style = style.curveStyle;

    for (auto *frame : m_keyframes)
        frame->setStyle(style);
}

void CurveItem::setInterpolation(Keyframe::Interpolation interpolation)
{
    if (m_keyframes.empty())
        return;

    KeyframeItem *prevItem = m_keyframes[0];
    for (int i = 1; i < m_keyframes.size(); ++i) {
        KeyframeItem *currItem = m_keyframes[i];
        if (currItem->selected()) {
            Keyframe prev = prevItem->keyframe();
            Keyframe curr = currItem->keyframe();
            CurveSegment segment(prev, curr);

            segment.setInterpolation(interpolation);
            prevItem->setKeyframe(segment.left());
            currItem->setKeyframe(segment.right());
        }

        prevItem = currItem;
    }
    setDirty(false);
    emit curveChanged(id(), curve());
}

void CurveItem::toggleUnified()
{
    if (m_keyframes.empty())
        return;

    for (auto *frame : m_keyframes) {
        if (frame->selected())
            frame->toggleUnified();
    }
    emit curveChanged(id(), curve());
}

void CurveItem::connect(GraphicsScene *scene)
{
    QObject::connect(this, &CurveItem::curveChanged, scene, &GraphicsScene::curveChanged);

    QObject::connect(this, &CurveItem::keyframeMoved, scene, &GraphicsScene::keyframeMoved);
    QObject::connect(this, &CurveItem::handleMoved, scene, &GraphicsScene::handleMoved);
}

void CurveItem::insertKeyframeByTime(double time)
{
    if (locked())
        return;

    AnimationCurve acurve = curve();
    acurve.insert(time);
    setCurve(acurve);

    emit curveChanged(id(), curve());
}

void CurveItem::deleteSelectedKeyframes()
{
    for (auto *&item : m_keyframes) {
        if (item->selected()) {
            delete item;
            item = nullptr;
        }
    }
    auto isNullptr = [](KeyframeItem *frame) { return frame == nullptr; };
    auto iter = std::remove_if(m_keyframes.begin(), m_keyframes.end(), isNullptr);
    m_keyframes.erase(iter, m_keyframes.end());
    restore();

    emitCurveChanged();
}

void CurveItem::emitCurveChanged()
{
    setDirty(true);
    update();
}

} // End namespace DesignTools.
