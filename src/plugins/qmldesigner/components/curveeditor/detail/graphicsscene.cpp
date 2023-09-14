// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "graphicsscene.h"
#include "animationcurve.h"
#include "curveitem.h"
#include "graphicsview.h"
#include "handleitem.h"

#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <QGraphicsSceneMouseEvent>

#include <cmath>

namespace QmlDesigner {

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

bool GraphicsScene::hasEditableSegment(double time) const
{
    for (auto *curve : m_curves) {
        if (curve->hasEditableSegment(time))
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

double GraphicsScene::animationRangeMin() const
{
    if (GraphicsView *gview = graphicsView())
        return gview->minimumTime();

    return minimumTime();
}

double GraphicsScene::animationRangeMax() const
{
    if (GraphicsView *gview = graphicsView())
        return gview->maximumTime();

    return maximumTime();
}

QRectF GraphicsScene::rect() const
{
    QRectF rect;
    for (auto *curve : curves())
        rect |= curve->boundingRect();

    return rect;
}

QList<CurveItem *> GraphicsScene::curves() const
{
    return m_curves;
}

QList<CurveItem *> GraphicsScene::selectedCurves() const
{
    QList<CurveItem *> out;
    for (auto *curve : m_curves) {
        if (curve->hasSelectedKeyframe())
            out.push_back(curve);
    }
    return out;
}

QList<KeyframeItem *> GraphicsScene::keyframes() const
{
    QList<KeyframeItem *> out;
    for (auto *curve : m_curves)
        out.append(curve->keyframes());

    return out;
}

QList<KeyframeItem *> GraphicsScene::selectedKeyframes() const
{
    QList<KeyframeItem *> out;
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

void GraphicsScene::setDirty(bool dirty)
{
    m_dirty = dirty;
}

void GraphicsScene::setIsMcu(bool isMcu)
{
    m_isMcu = isMcu;
    for (auto* curve : curves())
        curve->setIsMcu(isMcu);
}

void GraphicsScene::reset()
{
    m_curves.clear();
    clear();
}

void GraphicsScene::deleteSelectedKeyframes()
{
    m_dirty = true;
    for (auto *curve : std::as_const(m_curves))
        curve->deleteSelectedKeyframes();
}

void GraphicsScene::insertKeyframe(double time, bool all)
{
    if (!all) {
        for (auto *curve : std::as_const(m_curves)) {
            if (curve->isUnderMouse())
                curve->insertKeyframeByTime(std::round(time));
        }
        return;
    }

    for (auto *curve : std::as_const(m_curves))
        curve->insertKeyframeByTime(std::round(time));
}

void GraphicsScene::doNotMoveItems(bool val)
{
    m_doNotMoveItems = val;
}

void GraphicsScene::removeCurveItem(unsigned int id)
{
    CurveItem *tmp = nullptr;
    for (auto *curve : std::as_const(m_curves)) {
        if (curve->id() == id) {
            removeItem(curve);
            tmp = curve;
            break;
        }
    }

    if (tmp) {
        m_curves.removeOne(tmp);
        delete tmp;
    }

    m_dirty = true;
}

void GraphicsScene::addCurveItem(CurveItem *item)
{
    if (!item)
        return;

    for (auto *curve : std::as_const(m_curves)) {
        if (curve->id() == item->id()) {
            delete item;
            return;
        }
    }

    item->setIsMcu(m_isMcu);
    item->setDirty(false);
    item->connect(this);
    addItem(item);

    if (item->locked())
        m_curves.push_front(item);
    else
        m_curves.push_back(item);

    resetZValues();

    m_dirty = true;
}

void GraphicsScene::moveToBottom(CurveItem *item)
{
    if (!item)
        return;

    if (m_curves.removeAll(item) > 0) {
        m_curves.push_front(item);
        resetZValues();
    }
}

void GraphicsScene::moveToTop(CurveItem *item)
{
    if (!item)
        return;

    if (m_curves.removeAll(item) > 0) {
        m_curves.push_back(item);
        resetZValues();
    }
}

void GraphicsScene::setComponentTransform(const QTransform &transform)
{
    QRectF bounds;

    for (auto *curve : std::as_const(m_curves))
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
    for (auto *curve : std::as_const(m_curves)) {
        for (auto *keyframe : curve->keyframes()) {
            if (keyframe != movedItem && keyframe->selected())
                keyframe->moveKeyframe(direction);
        }
    }
}

void GraphicsScene::handleUnderMouse(HandleItem *handle)
{
    for (auto *curve : std::as_const(m_curves)) {
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

    for (auto *curve : std::as_const(m_curves)) {
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
    for (auto *curve : std::as_const(m_curves)) {
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

    for (auto *curve : std::as_const(m_curves)) {
        for (auto *handle : curve->handles()) {
            bool intersects = handle->contains(mouse);
            handle->setIsUnderMouse(intersects);
            if (intersects)
                hasHandle = true;
        }
    }

    if (hasHandle) {
        for (auto *curve : std::as_const(m_curves))
            curve->setIsUnderMouse(false);
    } else {
        for (auto *curve : std::as_const(m_curves))
            curve->setIsUnderMouse(curve->contains(mouseEvent->scenePos()));
    }
}

void GraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    QGraphicsScene::mouseReleaseEvent(mouseEvent);

    for (auto *curve : std::as_const(m_curves)) {
        // CurveItems might become invalid after a keyframe-drag operation.
        curve->restore();
        if (curve->isDirty()) {
            m_dirty = true;
            curve->setDirty(false);
            emit curveChanged(curve->id(), curve->curve(true));
        }
    }

    if (m_dirty)
        graphicsView()->setZoomY(0.0);
}

void GraphicsScene::focusOutEvent(QFocusEvent *focusEvent)
{
    QmlDesignerPlugin::emitUsageStatisticsTime(Constants::EVENT_CURVEDITOR_TIME,
                                               m_usageTimer.elapsed());
    QGraphicsScene::focusOutEvent(focusEvent);
}

void GraphicsScene::focusInEvent(QFocusEvent *focusEvent)
{
    m_usageTimer.restart();
    QGraphicsScene::focusInEvent(focusEvent);
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
        if (qFuzzyCompare(m_limits.height(), 0.0)) {
            auto tmp = CurveEditorStyle::defaultValueRange() / 2.0;
            m_limits.adjust(0.0, tmp, 0.0, -tmp);
        }

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

} // End namespace QmlDesigner.
