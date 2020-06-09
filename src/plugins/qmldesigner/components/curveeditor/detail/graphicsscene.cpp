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

#include "graphicsscene.h"
#include "animationcurve.h"
#include "curveitem.h"
#include "graphicsview.h"
#include "handleitem.h"

#include <QGraphicsSceneMouseEvent>

#include <cmath>

namespace DesignTools {

GraphicsScene::GraphicsScene(QObject *parent)
    : QGraphicsScene(parent)
    , m_curves()
    , m_dirty(true)
    , m_limits()
    , m_doNotMoveItems(false)
{}

GraphicsScene::~GraphicsScene()
{
    m_curves.clear();
}

bool GraphicsScene::empty() const
{
    return items().empty();
}

bool GraphicsScene::hasActiveKeyframe() const
{
    for (auto *curve : m_curves) {
        if (curve->hasActiveKeyframe())
            return true;
    }
    return false;
}

bool GraphicsScene::hasActiveHandle() const
{
    for (auto *curve : m_curves) {
        if (curve->hasActiveHandle())
            return true;
    }
    return false;
}

bool GraphicsScene::hasActiveItem() const
{
    return hasActiveKeyframe() || hasActiveHandle();
}

bool GraphicsScene::hasSelectedKeyframe() const
{
    for (auto *curve : m_curves) {
        if (curve->hasSelectedKeyframe())
            return true;
    }
    return false;
}

double GraphicsScene::minimumTime() const
{
    return limits().left();
}

double GraphicsScene::maximumTime() const
{
    return limits().right();
}

double GraphicsScene::minimumValue() const
{
    return limits().bottom();
}

double GraphicsScene::maximumValue() const
{
    return limits().top();
}

QRectF GraphicsScene::rect() const
{
    return sceneRect();
}

QVector<CurveItem *> GraphicsScene::curves() const
{
    return m_curves;
}

QVector<CurveItem *> GraphicsScene::selectedCurves() const
{
    QVector<CurveItem *> out;
    for (auto *curve : m_curves) {
        if (curve->hasSelectedKeyframe())
            out.push_back(curve);
    }
    return out;
}

QVector<KeyframeItem *> GraphicsScene::keyframes() const
{
    QVector<KeyframeItem *> out;
    for (auto *curve : m_curves)
        out.append(curve->keyframes());

    return out;
}

QVector<KeyframeItem *> GraphicsScene::selectedKeyframes() const
{
    QVector<KeyframeItem *> out;
    for (auto *curve : m_curves)
        out.append(curve->selectedKeyframes());

    return out;
}

CurveItem *GraphicsScene::findCurve(unsigned int id) const
{
    for (auto *curve : m_curves) {
        if (curve->id() == id)
            return curve;
    }
    return nullptr;
}

SelectableItem *GraphicsScene::intersect(const QPointF &pos) const
{
    auto hitTest = [pos](QGraphicsObject *item) {
        return item->mapRectToScene(item->boundingRect()).contains(pos);
    };

    const auto frames = keyframes();
    for (auto *frame : frames) {
        if (hitTest(frame))
            return frame;

        if (auto *leftHandle = frame->leftHandle()) {
            if (hitTest(leftHandle))
                return leftHandle;
        }

        if (auto *rightHandle = frame->rightHandle()) {
            if (hitTest(rightHandle))
                return rightHandle;
        }
    }
    return nullptr;
}

void GraphicsScene::reset()
{
    m_curves.clear();
    clear();
}

void GraphicsScene::deleteSelectedKeyframes()
{
    for (auto *curve : m_curves)
        curve->deleteSelectedKeyframes();
}

void GraphicsScene::insertKeyframe(double time, bool all)
{
    if (!all) {
        for (auto *curve : m_curves) {
            if (curve->isUnderMouse())
                curve->insertKeyframeByTime(std::round(time));
        }
        return;
    }

    for (auto *curve : m_curves)
        curve->insertKeyframeByTime(std::round(time));
}

void GraphicsScene::doNotMoveItems(bool val)
{
    m_doNotMoveItems = val;
}

void GraphicsScene::addCurveItem(CurveItem *item)
{
    m_dirty = true;
    item->setDirty(false);
    item->connect(this);
    addItem(item);

    if (item->locked())
        m_curves.push_front(item);
    else
        m_curves.push_back(item);

    resetZValues();
}

void GraphicsScene::moveToBottom(CurveItem *item)
{
    if (m_curves.removeAll(item) > 0) {
        m_curves.push_front(item);
        resetZValues();
    }
}

void GraphicsScene::moveToTop(CurveItem *item)
{
    if (m_curves.removeAll(item) > 0) {
        m_curves.push_back(item);
        resetZValues();
    }
}

void GraphicsScene::setComponentTransform(const QTransform &transform)
{
    QRectF bounds;

    for (auto *curve : m_curves)
        bounds = bounds.united(curve->setComponentTransform(transform));

    if (bounds.isNull()) {
        if (GraphicsView *gview = graphicsView())
            bounds = gview->defaultRasterRect();
    }

    if (bounds.isValid())
        setSceneRect(bounds);
}

void GraphicsScene::keyframeMoved(KeyframeItem *movedItem, const QPointF &direction)
{
    for (auto *curve : m_curves) {
        for (auto *keyframe : curve->keyframes()) {
            if (keyframe != movedItem && keyframe->selected())
                keyframe->moveKeyframe(direction);
        }
    }
}

void GraphicsScene::handleUnderMouse(HandleItem *handle)
{
    for (auto *curve : m_curves) {
        for (auto *keyframe : curve->keyframes()) {
            if (keyframe->selected())
                keyframe->setActivated(handle->isUnderMouse(), handle->slot());
        }
    }
}

void GraphicsScene::handleMoved(KeyframeItem *frame,
                                HandleItem::Slot handle,
                                double angle,
                                double deltaLength)
{
    if (m_doNotMoveItems)
        return;

    auto moveUnified = [handle, angle, deltaLength](KeyframeItem *key) {
        if (key->isUnified()) {
            if (handle == HandleItem::Slot::Left)
                key->moveHandle(HandleItem::Slot::Right, angle, deltaLength);
            else
                key->moveHandle(HandleItem::Slot::Left, angle, deltaLength);
        }
    };

    for (auto *curve : m_curves) {
        for (auto *keyframe : curve->keyframes()) {
            if (keyframe == frame)
                moveUnified(keyframe);
            else if (keyframe->selected()) {
                keyframe->moveHandle(handle, angle, deltaLength);
                moveUnified(keyframe);
            }
        }
    }
}

void GraphicsScene::setPinned(uint id, bool pinned)
{
    if (CurveItem *curve = findCurve(id))
        curve->setPinned(pinned);
}

std::vector<CurveItem *> GraphicsScene::takePinnedItems()
{
    std::vector<CurveItem *> out;
    for (auto *curve : m_curves) {
        if (curve->pinned())
            out.push_back(curve);
    }

    for (auto *curve : out) {
        curve->disconnect(this);
        m_curves.removeOne(curve);
        removeItem(curve);
    }

    return out;
}

void GraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    QGraphicsScene::mouseMoveEvent(mouseEvent);

    QPointF mouse = mouseEvent->scenePos();
    bool hasHandle = false;

    for (auto *curve : m_curves) {
        for (auto *handle : curve->handles()) {
            bool intersects = handle->contains(mouse);
            handle->setIsUnderMouse(intersects);
            if (intersects)
                hasHandle = true;
        }
    }

    if (hasHandle) {
        for (auto *curve : m_curves)
            curve->setIsUnderMouse(false);
    } else {
        for (auto *curve : m_curves)
            curve->setIsUnderMouse(curve->contains(mouseEvent->scenePos()));
    }
}

void GraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    QGraphicsScene::mouseReleaseEvent(mouseEvent);

    for (auto *curve : m_curves) {
        // CurveItems might become invalid after a keyframe-drag operation.
        curve->restore();
        if (curve->isDirty()) {
            m_dirty = true;
            curve->setDirty(false);
            emit curveChanged(curve->id(), curve->curve());
        }
    }

    if (m_dirty)
        graphicsView()->setZoomY(0.0);
}

GraphicsView *GraphicsScene::graphicsView() const
{
    const QList<QGraphicsView *> viewList = views();
    if (viewList.size() == 1) {
        if (GraphicsView *gview = qobject_cast<GraphicsView *>(viewList.at(0)))
            return gview;
    }
    return nullptr;
}

QRectF GraphicsScene::limits() const
{
    if (m_dirty) {
        QPointF min(std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
        QPointF max(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest());

        for (auto *curveItem : m_curves) {
            auto curve = curveItem->resolvedCurve();
            if (min.x() > curve.minimumTime())
                min.rx() = curve.minimumTime();

            if (min.y() > curve.minimumValue())
                min.ry() = curve.minimumValue();

            if (max.x() < curve.maximumTime())
                max.rx() = curve.maximumTime();

            if (max.y() < curve.maximumValue())
                max.ry() = curve.maximumValue();
        }

        m_limits = QRectF(QPointF(min.x(), max.y()), QPointF(max.x(), min.y()));
        m_dirty = false;
    }
    return m_limits;
}

void GraphicsScene::resetZValues()
{
    qreal z = 0.0;
    for (auto *curve : curves()) {
        curve->setZValue(z);
        z += 1.0;
    }
}

} // End namespace DesignTools.
