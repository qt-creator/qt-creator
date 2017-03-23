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

TimelineRenderer::TimelineRendererPrivate::TimelineRendererPrivate() : lastState(0)
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
    TimelineAbstractRenderer(*(new TimelineRendererPrivate), parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
}

void TimelineRenderer::TimelineRendererPrivate::resetCurrentSelection()
{
    currentEventIndex = -1;
    currentRow = -1;
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
    d->findCurrentSelection(event->pos().x(), event->pos().y(), width());
    setSelectedItem(d->currentEventIndex);
}

void TimelineRenderer::mouseMoveEvent(QMouseEvent *event)
{
    event->setAccepted(false);
}

void TimelineRenderer::hoverMoveEvent(QHoverEvent *event)
{
    Q_D(TimelineRenderer);
    if (!d->selectionLocked) {
        d->findCurrentSelection(event->pos().x(), event->pos().y(), width());
        if (d->currentEventIndex != -1)
            setSelectedItem(d->currentEventIndex);
    }
    if (d->currentEventIndex == -1)
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

TimelineRenderer::TimelineRendererPrivate::MatchResult
TimelineRenderer::TimelineRendererPrivate::checkMatch(MatchParameters *params, int index,
                                                      qint64 itemStart, qint64 itemEnd)
{
    const qint64 offset = qAbs(itemEnd - params->exactTime) + qAbs(itemStart - params->exactTime);
    if (offset >= params->bestOffset)
        return NoMatch;

    // match
    params->bestOffset = offset;
    currentEventIndex = index;

    // Exact match. If we can get better than this, then we have multiple overlapping
    // events in one row. There is no point in sorting those out as you cannot properly
    // discern them anyway.
    return (itemEnd >= params->exactTime && itemStart <= params->exactTime)
            ? ExactMatch : ApproximateMatch;
}

TimelineRenderer::TimelineRendererPrivate::MatchResult
TimelineRenderer::TimelineRendererPrivate::matchForward(MatchParameters *params, int index)
{
    if (index < 0)
        return NoMatch;

    if (index >= model->count())
        return Cutoff;

    if (model->row(index) != currentRow)
        return NoMatch;

    const qint64 itemEnd = model->endTime(index);
    if (itemEnd < params->startTime)
        return NoMatch;

    const qint64 itemStart = model->startTime(index);
    if (itemStart > params->endTime)
        return Cutoff;

    // Further iteration will only increase the startOffset.
    if (itemStart - params->exactTime >= params->bestOffset)
        return Cutoff;

    return checkMatch(params, index, itemStart, itemEnd);
}

TimelineRenderer::TimelineRendererPrivate::MatchResult
TimelineRenderer::TimelineRendererPrivate::matchBackward(MatchParameters *params, int index)
{
    if (index < 0)
        return Cutoff;

    if (index >= model->count())
        return NoMatch;

    if (model->row(index) != currentRow)
        return NoMatch;

    const qint64 itemStart = model->startTime(index);
    if (itemStart > params->endTime)
        return NoMatch;

    // There can be small events that don't reach the cursor position after large events
    // that do but are in a different row. In that case, the parent index will be valid and will
    // point to the large event. If that is also outside the range, we are really done.
    const qint64 itemEnd = model->endTime(index);
    if (itemEnd < params->startTime) {
        const int parentIndex = model->parentIndex(index);
        const qint64 parentEnd = parentIndex == -1 ? itemEnd : model->endTime(parentIndex);
        return (parentEnd < params->startTime) ? Cutoff : NoMatch;
    }

    if (params->exactTime - itemStart >= params->bestOffset) {
        // We cannot get better anymore as the startTimes are totally ordered.
        // Thus, the startOffset will only get bigger and we're only adding a
        // positive number (end offset) in checkMatch() when comparing with bestOffset.
        return Cutoff;
    }

    return checkMatch(params, index, itemStart, itemEnd);
}

void TimelineRenderer::TimelineRendererPrivate::findCurrentSelection(int mouseX, int mouseY,
                                                                     int width)
{
    if (!zoomer || !model || width < 1)
        return;

    qint64 duration = zoomer->windowDuration();
    if (duration <= 0)
        return;

    MatchParameters params;

    // Make the "selected" area 3 pixels wide by adding/subtracting 1 to catch very narrow events.
    params.startTime = (mouseX - 1) * duration / width + zoomer->windowStart();
    params.endTime = (mouseX + 1) * duration / width + zoomer->windowStart();
    params.exactTime = (params.startTime + params.endTime) / 2;
    const int row = rowFromPosition(mouseY);

    // already covered? Only make sure d->selectedItem is correct.
    if (currentEventIndex != -1 &&
            params.exactTime >= model->startTime(currentEventIndex) &&
            params.exactTime < model->endTime(currentEventIndex) &&
            row == currentRow) {
        return;
    }

    currentRow = row;
    currentEventIndex = -1;

    const int middle = model->bestIndex(params.exactTime);
    if (middle == -1)
        return;

    params.bestOffset = std::numeric_limits<qint64>::max();
    const qint64 itemStart = model->startTime(middle);
    const qint64 itemEnd = model->endTime(middle);
    if (model->row(middle) == row && itemEnd >= params.startTime && itemStart <= params.endTime) {
        if (checkMatch(&params, middle, itemStart, itemEnd) == ExactMatch)
            return;
    }

    MatchResult forward = NoMatch;
    MatchResult backward = NoMatch;
    for (int offset = 1; forward != Cutoff || backward != Cutoff; ++offset) {
        if (backward != Cutoff
                && (backward = matchBackward(&params, middle - offset)) == ExactMatch) {
            return;
        }
        if (forward != Cutoff
                && (forward = matchForward(&params, middle + offset)) == ExactMatch) {
            return;
        }
    }
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
