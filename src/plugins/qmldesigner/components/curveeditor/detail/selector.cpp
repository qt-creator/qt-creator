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
#include "selector.h"
#include "graphicsscene.h"
#include "graphicsview.h"
#include "keyframeitem.h"
#include "playhead.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPoint>
#include <QRectF>

#include <cmath>

namespace DesignTools {

Selector::Selector() {}

void Selector::paint(QPainter *painter)
{
    QPen pen(Qt::white);

    painter->save();
    painter->setPen(pen);

    if (!m_lasso.isEmpty())
        painter->drawPath(m_lasso);

    if (!m_rect.isNull())
        painter->drawRect(m_rect);

    painter->restore();
}

void Selector::mousePress(QMouseEvent *event, GraphicsView *view, GraphicsScene *scene)
{
    m_shortcut = Shortcut(event);

    QPointF click = view->globalToScene(event->globalPos());

    if (SelectableItem *sitem = scene->intersect(click)) {
        KeyframeItem *kitem = qobject_cast<KeyframeItem *>(sitem);
        if (HandleItem *hitem = qobject_cast<HandleItem *>(sitem))
            kitem = hitem->keyframe();

        if (kitem && !kitem->selected()) {
            if (select(SelectionTool::Undefined, click, scene))
                applyPreSelection(scene);
        }
    } else {
        if (select(SelectionTool::Undefined, click, scene))
            applyPreSelection(scene);

        // Init selection tools.
        m_mouseInit = event->globalPos();
        m_mouseCurr = event->globalPos();

        m_lasso = QPainterPath(click);
        m_lasso.closeSubpath();

        m_rect = QRectF(click, click);
    }
}

void Selector::mouseMove(QMouseEvent *event,
                         GraphicsView *view,
                         GraphicsScene *scene,
                         Playhead &playhead)
{
    if (m_mouseInit.isNull())
        return;

    if ((event->globalPos() - m_mouseInit).manhattanLength() < QApplication::startDragDistance())
        return;

    QPointF delta = event->globalPos() - m_mouseCurr;
    if (m_shortcut == m_shortcuts.newSelection || m_shortcut == m_shortcuts.addToSelection
        || m_shortcut == m_shortcuts.removeFromSelection
        || m_shortcut == m_shortcuts.toggleSelection) {
        if (scene->hasActiveItem())
            return;

        select(m_tool, view->globalToScene(event->globalPos()), scene);

        event->accept();
        view->viewport()->update();

    } else if (m_shortcut == m_shortcuts.zoom) {
        double bigger = std::abs(delta.x()) > std::abs(delta.y()) ? delta.x() : delta.y();
        double factor = bigger / view->width();
        view->setZoomX(view->zoomX() + factor, m_mouseInit);
        m_mouseCurr = event->globalPos();
        event->accept();

    } else if (m_shortcut == m_shortcuts.pan) {
        view->scrollContent(-delta.x(), -delta.y());
        playhead.resize(view);
        m_mouseCurr = event->globalPos();
    }
}

void Selector::mouseRelease(QMouseEvent *event, GraphicsScene *scene)
{
    Q_UNUSED(event)

    applyPreSelection(scene);

    m_shortcut = Shortcut();
    m_mouseInit = QPoint();
    m_mouseCurr = QPoint();
    m_lasso = QPainterPath();
    m_rect = QRectF();
}

bool Selector::isOverMovableItem(const QPointF &pos, GraphicsScene *scene)
{
    auto intersect = [pos](QGraphicsObject *item) {
        return item->mapRectToScene(item->boundingRect()).contains(pos);
    };

    const auto frames = scene->keyframes();
    for (auto *frame : frames) {
        if (intersect(frame))
            return true;

        if (auto *leftHandle = frame->leftHandle()) {
            if (intersect(leftHandle))
                return true;
        }

        if (auto *rightHandle = frame->rightHandle()) {
            if (intersect(rightHandle))
                return true;
        }
    }
    return false;
}

bool Selector::isOverSelectedKeyframe(const QPointF &pos, GraphicsScene *scene)
{
    const auto frames = scene->selectedKeyframes();
    for (auto *frame : frames) {
        QRectF frameRect = frame->mapRectToScene(frame->boundingRect());
        if (frameRect.contains(pos))
            return true;
    }
    return false;
}

bool Selector::select(const SelectionTool &tool, const QPointF &pos, GraphicsScene *scene)
{
    auto selectWidthTool = [this,
                            tool](SelectionMode mode, const QPointF &pos, GraphicsScene *scene) {
        switch (tool) {
        case SelectionTool::Lasso:
            return lassoSelection(mode, pos, scene);
        case SelectionTool::Rectangle:
            return rectangleSelection(mode, pos, scene);
        default:
            return pressSelection(mode, pos, scene);
        }
    };

    if (m_shortcut == m_shortcuts.newSelection) {
        clearSelection(scene);
        return selectWidthTool(SelectionMode::New, pos, scene);
    } else if (m_shortcut == m_shortcuts.addToSelection) {
        return selectWidthTool(SelectionMode::Add, pos, scene);
    } else if (m_shortcut == m_shortcuts.removeFromSelection) {
        return selectWidthTool(SelectionMode::Remove, pos, scene);
    } else if (m_shortcut == m_shortcuts.toggleSelection) {
        return selectWidthTool(SelectionMode::Toggle, pos, scene);
    }

    return false;
}

bool Selector::pressSelection(SelectionMode mode, const QPointF &pos, GraphicsScene *scene)
{
    bool out = false;
    const auto itemList = scene->items();
    for (auto *item : itemList) {
        if (auto *frame = qgraphicsitem_cast<KeyframeItem *>(item)) {
            QRectF itemRect = frame->mapRectToScene(frame->boundingRect());
            if (itemRect.contains(pos)) {
                frame->setPreselected(mode);
                out = true;
            }
        }

        if (auto *handle = qgraphicsitem_cast<HandleItem *>(item)) {
            QRectF itemRect = handle->mapRectToScene(handle->boundingRect());
            if (itemRect.contains(pos)) {
                if (auto *frame = handle->keyframe()) {
                    frame->setPreselected(mode);
                    out = true;
                }
            }
        }
    }
    return out;
}

bool Selector::rectangleSelection(SelectionMode mode, const QPointF &pos, GraphicsScene *scene)
{
    bool out = false;
    m_rect.setBottomRight(pos);
    const auto itemList = scene->items();
    for (auto *item : itemList) {
        if (auto *keyframeItem = qgraphicsitem_cast<KeyframeItem *>(item)) {
            if (m_rect.contains(keyframeItem->pos())) {
                keyframeItem->setPreselected(mode);
                out = true;
            } else {
                keyframeItem->setPreselected(SelectionMode::Undefined);
            }
        }
    }
    return out;
}

bool Selector::lassoSelection(SelectionMode mode, const QPointF &pos, GraphicsScene *scene)
{
    bool out = false;
    m_lasso.lineTo(pos);
    const auto itemList = scene->items();
    for (auto *item : itemList) {
        if (auto *keyframeItem = qgraphicsitem_cast<KeyframeItem *>(item)) {
            if (m_lasso.contains(keyframeItem->pos())) {
                keyframeItem->setPreselected(mode);
                out = true;
            } else {
                keyframeItem->setPreselected(SelectionMode::Undefined);
            }
        }
    }
    return out;
}

void Selector::clearSelection(GraphicsScene *scene)
{
    const auto itemList = scene->items();
    for (auto *item : itemList) {
        if (auto *frameItem = qgraphicsitem_cast<KeyframeItem *>(item)) {
            frameItem->setPreselected(SelectionMode::Clear);
            frameItem->applyPreselection();
            frameItem->setActivated(false, HandleItem::Slot::Left);
            frameItem->setActivated(false, HandleItem::Slot::Right);
        }
    }
}

void Selector::applyPreSelection(GraphicsScene *scene)
{
    const auto itemList = scene->items();
    for (auto *item : itemList) {
        if (auto *keyframeItem = qgraphicsitem_cast<KeyframeItem *>(item))
            keyframeItem->applyPreselection();
    }
}

} // End namespace DesignTools.
