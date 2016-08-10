/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
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

#include "timelinerenderer_p.h"
#include "timelinerenderpass.h"
#include "timelinenotesmodel.h"
#include "timelineitemsrenderpass.h"
#include "timelineselectionrenderpass.h"
#include "timelinenotesrenderpass.h"

#include <QElapsedTimer>
#include <QQmlContext>
#include <QQmlProperty>
#include <QTimer>
#include <QPixmap>
#include <QVarLengthArray>
#include <QSGTransformNode>
#include <QSGSimpleRectNode>

#include <math.h>

namespace Timeline {

TimelineRenderer::TimelineRendererPrivate::TimelineRendererPrivate(TimelineRenderer *q) :
    lastState(0), q_ptr(q)
{
    resetCurrentSelection();
}

TimelineRenderer::TimelineRendererPrivate::~TimelineRendererPrivate()
{
    clear();
}

void TimelineRenderer::TimelineRendererPrivate::clear()
{
    for (auto i = renderStates.begin(); i != renderStates.end(); ++i)
        qDeleteAll(*i);
    renderStates.clear();
    lastState = 0;
}

TimelineRenderer::TimelineRenderer(QQuickItem *parent) :
    TimelineAbstractRenderer(*(new TimelineRendererPrivate(this)), parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
}

void TimelineRenderer::TimelineRendererPrivate::resetCurrentSelection()
{
    currentSelection.startTime = -1;
    currentSelection.endTime = -1;
    currentSelection.row = -1;
    currentSelection.eventIndex = -1;
}

TimelineRenderState *TimelineRenderer::TimelineRendererPrivate::findRenderState()
{
    int newLevel = 0;
    qint64 newOffset = 0;
    int level;
    qint64 offset;

    qint64 newStart = zoomer->traceStart();
    qint64 newEnd = zoomer->traceEnd();
    qint64 start;
    qint64 end;
    do {
        level = newLevel;
        offset = newOffset;
        start = newStart;
        end = newEnd;

        newLevel = level + 1;
        qint64 range = zoomer->traceDuration() >> newLevel;
        newOffset = (zoomer->windowStart() - zoomer->traceStart() + range / 2) / range;
        newStart = zoomer->traceStart() + newOffset * range - range / 2;
        newEnd = newStart + range;
    } while (newStart < zoomer->windowStart() && newEnd > zoomer->windowEnd());


    if (renderStates.length() <= level)
        renderStates.resize(level + 1);
    TimelineRenderState *state = renderStates[level][offset];
    if (state == 0) {
        state = new TimelineRenderState(start, end, 1.0 / static_cast<qreal>(SafeFloatMax),
                                        renderPasses.size());
        renderStates[level][offset] = state;
    }
    return state;
}

QSGNode *TimelineRenderer::updatePaintNode(QSGNode *node, UpdatePaintNodeData *updatePaintNodeData)
{
    Q_D(TimelineRenderer);

    if (!d->model || d->model->hidden() || d->model->isEmpty() || !d->zoomer ||
            d->zoomer->windowDuration() <= 0) {
        delete node;
        return 0;
    }

    qreal spacing = width() / d->zoomer->windowDuration();

    if (d->modelDirty) {
        if (node)
            node->removeAllChildNodes();
        d->clear();
    }

    TimelineRenderState *state = d->findRenderState();

    int lastIndex = d->model->lastIndex(d->zoomer->windowEnd());
    int firstIndex = d->model->firstIndex(d->zoomer->windowStart());

    for (int i = 0; i < d->renderPasses.length(); ++i)
        state->setPassState(i, d->renderPasses[i]->update(this, state, state->passState(i),
                                                         firstIndex, lastIndex + 1,
                                                         state != d->lastState, spacing));

    if (state->isEmpty()) { // new state
        state->assembleNodeTree(d->model, TimelineModel::defaultRowHeight(),
                                TimelineModel::defaultRowHeight());
    } else if (d->rowHeightsDirty || state != d->lastState) {
        state->updateExpandedRowHeights(d->model, TimelineModel::defaultRowHeight(),
                                        TimelineModel::defaultRowHeight());
    }

    TimelineAbstractRenderer::updatePaintNode(0, updatePaintNodeData);
    d->lastState = state;

    QMatrix4x4 matrix;
    matrix.translate((state->start() - d->zoomer->windowStart()) * spacing, 0, 0);
    matrix.scale(spacing / state->scale(), 1, 1);

    return state->finalize(node, d->model->expanded(), matrix);
}

void TimelineRenderer::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
}

int TimelineRenderer::TimelineRendererPrivate::rowFromPosition(int y) const
{
    if (!model->expanded())
        return y / TimelineModel::defaultRowHeight();

    int ret = 0;
    for (int row = 0; row < model->expandedRowCount(); ++row) {
        y -= model->expandedRowHeight(row);
        if (y <= 0) return ret;
        ++ret;
    }

    return ret;
}

void TimelineRenderer::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(TimelineRenderer);
    Q_UNUSED(event);
    if (d->model && !d->model->isEmpty())
        d->manageClicked();
}

void TimelineRenderer::mouseMoveEvent(QMouseEvent *event)
{
    event->setAccepted(false);
}

void TimelineRenderer::hoverMoveEvent(QHoverEvent *event)
{
    Q_D(TimelineRenderer);
    d->manageHovered(event->pos().x(), event->pos().y());
    if (d->currentSelection.eventIndex == -1)
        event->setAccepted(false);
}

void TimelineRenderer::wheelEvent(QWheelEvent *event)
{
    // ctrl-wheel means zoom
    if (event->modifiers() & Qt::ControlModifier) {
        event->setAccepted(true);
        TimelineZoomControl *zoom = zoomer();

        int degrees = (event->angleDelta().x() + event->angleDelta().y()) / 8;
        const qint64 circle = 360;
        qint64 mouseTime = event->pos().x() * zoom->windowDuration() / width() +
                zoom->windowStart();
        qint64 beforeMouse = (mouseTime - zoom->rangeStart()) * (circle - degrees) / circle;
        qint64 afterMouse = (zoom->rangeEnd() - mouseTime) * (circle - degrees) / circle;

        qint64 newStart = qBound(zoom->traceStart(), zoom->traceEnd(), mouseTime - beforeMouse);
        if (newStart + zoom->minimumRangeLength() > zoom->traceEnd())
            return; // too close to trace end

        qint64 newEnd = qBound(newStart + zoom->minimumRangeLength(), zoom->traceEnd(),
                               mouseTime + afterMouse);

        zoom->setRange(newStart, newEnd);
    } else {
        TimelineAbstractRenderer::wheelEvent(event);
    }
}

void TimelineRenderer::TimelineRendererPrivate::manageClicked()
{
    Q_Q(TimelineRenderer);
    if (currentSelection.eventIndex != -1)
        q->setSelectedItem(currentSelection.eventIndex);
    else
        q->setSelectedItem(-1);
}

void TimelineRenderer::TimelineRendererPrivate::manageHovered(int mouseX, int mouseY)
{
    Q_Q(TimelineRenderer);
    if (!zoomer || !model || q->width() < 1)
        return;

    qint64 duration = zoomer->windowDuration();
    if (duration <= 0)
        return;

    // Make the "selected" area 3 pixels wide by adding/subtracting 1 to catch very narrow events.
    qint64 startTime = (mouseX - 1) * duration / q->width() + zoomer->windowStart();
    qint64 endTime = (mouseX + 1) * duration / q->width() + zoomer->windowStart();
    qint64 exactTime = (startTime + endTime) / 2;
    int row = rowFromPosition(mouseY);

    // already covered? Only recheck selectionLocked and make sure d->selectedItem is correct.
    if (currentSelection.eventIndex != -1 &&
            exactTime >= currentSelection.startTime &&
            exactTime < currentSelection.endTime &&
            row == currentSelection.row) {
        if (!selectionLocked)
            q->setSelectedItem(currentSelection.eventIndex);
        return;
    }

    // find if there's items in the time range
    int eventFrom = model->firstIndex(startTime);
    int eventTo = model->lastIndex(endTime);

    currentSelection.eventIndex = -1;
    if (eventFrom == -1 || eventTo < eventFrom || eventTo >= model->count())
        return;

    // find if we are in the right column
    qint64 bestOffset = std::numeric_limits<qint64>::max();
    for (int i=eventTo; i>=eventFrom; --i) {
        if (model->row(i) != row)
            continue;

        // There can be small events that don't reach the cursor position after large events
        // that do but are in a different row.
        qint64 itemEnd = model->endTime(i);
        if (itemEnd < startTime)
            continue;

        qint64 itemStart = model->startTime(i);

        qint64 startOffset = exactTime - itemStart;
        if (startOffset >= bestOffset) {
            // We cannot get better anymore as the startTimes are totally ordered and we're moving
            // backwards. Thus, the startOffset will only get bigger and we're only adding a
            // positive number (end offset) below when comparing with bestOffset.
            break;
        }

        qint64 offset = qAbs(itemEnd - exactTime) + qAbs(startOffset);
        if (offset >= bestOffset)
            continue;

        // match
        currentSelection.eventIndex = i;
        currentSelection.startTime = itemStart;
        currentSelection.endTime = itemEnd;
        currentSelection.row = row;

        // Exact match. If we can get better than this, then we have multiple overlapping
        // events in one row. There is no point in sorting those out as you cannot properly
        // discern them anyway.
        if (itemEnd >= exactTime && itemStart <= exactTime)
            break;

        bestOffset = offset;
    }
    if (!selectionLocked && currentSelection.eventIndex != -1)
        q->setSelectedItem(currentSelection.eventIndex);
}

void TimelineRenderer::clearData()
{
    Q_D(TimelineRenderer);
    d->resetCurrentSelection();
    setSelectedItem(-1);
    setSelectionLocked(true);
}

void TimelineRenderer::selectNextFromSelectionId(int selectionId)
{
    Q_D(TimelineRenderer);
    setSelectedItem(d->model->nextItemBySelectionId(selectionId, d->zoomer->rangeStart(),
                                                   d->selectedItem));
}

void TimelineRenderer::selectPrevFromSelectionId(int selectionId)
{
    Q_D(TimelineRenderer);
    setSelectedItem(d->model->prevItemBySelectionId(selectionId, d->zoomer->rangeStart(),
                                                   d->selectedItem));
}

} // namespace Timeline
