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

namespace DesignTools {

GraphicsScene::GraphicsScene(QObject *parent)
    : QGraphicsScene(parent)
    , m_dirty(true)
    , m_limits()
{}

bool GraphicsScene::empty() const
{
    return items().empty();
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

void GraphicsScene::addCurveItem(CurveItem *item)
{
    m_dirty = true;
    item->setDirty(false);
    item->connect(this);
    addItem(item);
}

void GraphicsScene::setComponentTransform(const QTransform &transform)
{
    QRectF bounds;
    const auto itemList = items();
    for (auto *item : itemList) {
        if (auto *curveItem = qgraphicsitem_cast<CurveItem *>(item))
            bounds = bounds.united(curveItem->setComponentTransform(transform));
    }

    if (bounds.isNull()) {
        if (GraphicsView *gview = graphicsView())
            bounds = gview->defaultRasterRect();
    }

    if (bounds.isValid())
        setSceneRect(bounds);
}

void GraphicsScene::keyframeMoved(KeyframeItem *movedItem, const QPointF &direction)
{
    const auto itemList = items();
    for (auto *item : itemList) {
        if (item == movedItem)
            continue;

        if (auto *frameItem = qgraphicsitem_cast<KeyframeItem *>(item)) {
            if (frameItem->selected())
                frameItem->moveKeyframe(direction);
        }
    }
}

void GraphicsScene::handleMoved(KeyframeItem *frame,
                                HandleSlot handle,
                                double angle,
                                double deltaLength)
{
    const auto itemList = items();
    for (auto *item : itemList) {
        if (item == frame)
            continue;

        if (auto *frameItem = qgraphicsitem_cast<KeyframeItem *>(item)) {
            if (frameItem->selected())
                frameItem->moveHandle(handle, angle, deltaLength);
        }
    }
}

void GraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    QGraphicsScene::mouseMoveEvent(mouseEvent);

    if (hasActiveItem())
        return;

    const auto itemList = items();
    for (auto *item : itemList) {
        if (auto *curveItem = qgraphicsitem_cast<CurveItem *>(item))
            curveItem->setIsUnderMouse(curveItem->contains(mouseEvent->scenePos()));
    }
}

void GraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    QGraphicsScene::mouseReleaseEvent(mouseEvent);

    const auto itemList = items();
    for (auto *item : itemList) {
        if (auto *curveItem = qgraphicsitem_cast<CurveItem *>(item)) {
            // CurveItems might become invalid after a keyframe-drag operation.
            curveItem->restore();

            if (curveItem->isDirty()) {
                m_dirty = true;
                curveItem->setDirty(false);
                emit curveChanged(curveItem->id(), curveItem->curve());
            }
        }
    }

    if (m_dirty)
        graphicsView()->setZoomY(0.0);
}

bool GraphicsScene::hasActiveKeyframe() const
{
    const auto itemList = items();
    for (auto *item : itemList) {
        if (auto *kitem = qgraphicsitem_cast<KeyframeItem *>(item)) {
            if (kitem->activated())
                return true;
        }
    }
    return false;
}

bool GraphicsScene::hasActiveHandle() const
{
    const auto itemList = items();
    for (auto *item : itemList) {
        if (auto *hitem = qgraphicsitem_cast<HandleItem *>(item)) {
            if (hitem->activated())
                return true;
        }
    }
    return false;
}

bool GraphicsScene::hasActiveItem() const
{
    return hasActiveKeyframe() || hasActiveHandle();
}

GraphicsView *GraphicsScene::graphicsView() const
{
    const QList<QGraphicsView *> viewList = views();
    for (auto &&view : viewList) {
        if (GraphicsView *gview = qobject_cast<GraphicsView *>(view))
            return gview;
    }
    return nullptr;
}

QRectF GraphicsScene::limits() const
{
    if (m_dirty) {
        QPointF min(std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
        QPointF max(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest());

        const auto itemList = items();
        for (auto *item : itemList) {
            if (auto *curveItem = qgraphicsitem_cast<CurveItem *>(item)) {
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
        }

        m_limits = QRectF(QPointF(min.x(), max.y()), QPointF(max.x(), min.y()));
        m_dirty = false;
    }
    return m_limits;
}

} // End namespace DesignTools.
