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

#include <QPainter>
#include <QPainterPath>

#include <cmath>

namespace DesignTools {

CurveItem::CurveItem(QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_id(0)
    , m_style()
    , m_transform()
    , m_keyframes()
    , m_underMouse(false)
    , m_itemDirty(false)
    , m_pathDirty(true)
{}

CurveItem::CurveItem(unsigned int id, const AnimationCurve &curve, QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_id(id)
    , m_style()
    , m_transform()
    , m_keyframes()
    , m_underMouse(false)
    , m_itemDirty(false)
    , m_pathDirty(true)
{
    setAcceptHoverEvents(true);

    setFlag(QGraphicsItem::ItemIsMovable, false);

    auto emitCurveChanged = [this]() {
        m_itemDirty = true;
        m_pathDirty = true;
        update();
    };

    for (auto frame : curve.keyframes()) {
        auto *item = new KeyframeItem(frame, this);
        QObject::connect(item, &KeyframeItem::redrawCurve, emitCurveChanged);
        m_keyframes.push_back(item);
    }
}

CurveItem::~CurveItem() {}

int CurveItem::type() const
{
    return Type;
}

QRectF CurveItem::boundingRect() const
{
    auto bbox = [](QRectF &bounds, const Keyframe &frame) {
        grow(bounds, frame.position());
        grow(bounds, frame.leftHandle());
        grow(bounds, frame.rightHandle());
    };

    QRectF bounds;
    for (auto *item : m_keyframes)
        bbox(bounds, item->keyframe());

    return m_transform.mapRect(bounds);
}

bool CurveItem::contains(const QPointF &point) const
{
    bool valid = false;
    QPointF transformed(m_transform.inverted(&valid).map(point));

    double width = std::abs(20.0 / scaleY(m_transform));

    if (valid)
        return curve().intersects(transformed, width);

    return false;
}

void CurveItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (m_keyframes.size() > 1) {
        QPen pen = painter->pen();
        QColor col = m_underMouse ? Qt::red : m_style.color;

        pen.setWidthF(m_style.width);
        pen.setColor(hasSelection() ? m_style.selectionColor : col);

        painter->save();
        painter->setPen(pen);
        painter->drawPath(path());

        painter->restore();
    }
}

bool CurveItem::isDirty() const
{
    return m_itemDirty;
}

bool CurveItem::hasSelection() const
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

QPainterPath CurveItem::path() const
{
    if (m_pathDirty) {
        Keyframe previous = m_keyframes.front()->keyframe();
        Keyframe current;

        m_path = QPainterPath(m_transform.map(previous.position()));
        for (size_t i = 1; i < m_keyframes.size(); ++i) {
            current = m_keyframes[i]->keyframe();

            if (previous.rightHandle().isNull() || current.leftHandle().isNull()) {
                m_path.lineTo(m_transform.map(current.position()));
            } else {
                m_path.cubicTo(
                    m_transform.map(previous.rightHandle()),
                    m_transform.map(current.leftHandle()),
                    m_transform.map(current.position()));
            }

            previous = current;
        }
        m_pathDirty = false;
    }

    return m_path;
}

AnimationCurve CurveItem::curve() const
{
    std::vector<Keyframe> out;
    out.reserve(m_keyframes.size());
    for (auto item : m_keyframes)
        out.push_back(item->keyframe());

    return out;
}

void CurveItem::setDirty(bool dirty)
{
    m_itemDirty = dirty;
}

QRectF CurveItem::setComponentTransform(const QTransform &transform)
{
    m_pathDirty = true;

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

void CurveItem::connect(GraphicsScene *scene)
{
    for (auto *frame : m_keyframes) {
        QObject::connect(frame, &KeyframeItem::keyframeMoved, scene, &GraphicsScene::keyframeMoved);
        QObject::connect(frame, &KeyframeItem::handleMoved, scene, &GraphicsScene::handleMoved);
    }
}

void CurveItem::setIsUnderMouse(bool under)
{
    if (under != m_underMouse) {
        m_underMouse = under;
        update();
    }
}

} // End namespace DesignTools.
