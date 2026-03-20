// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinerenderer.h"
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

TimelineRenderer::TimelineRenderer(QQuickItem *parent)
    : TimelineAbstractRenderer(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    resetCurrentSelection();
}

TimelineRenderer::~TimelineRenderer()
{
    clear();
}

void TimelineRenderer::clear()
{
    for (auto i = m_renderStates.begin(); i != m_renderStates.end(); ++i)
        qDeleteAll(*i);
    m_renderStates.clear();
    m_lastState = nullptr;
}

void TimelineRenderer::resetCurrentSelection()
{
    m_currentEventIndex = -1;
    m_currentRow = -1;
}

TimelineRenderState *TimelineRenderer::findRenderState()
{
    int newLevel = 0;
    qint64 newOffset = 0;
    int level;
    qint64 offset;

    qint64 newStart = m_zoomer->traceStart();
    qint64 newEnd = m_zoomer->traceEnd();
    qint64 start;
    qint64 end;
    do {
        level = newLevel;
        offset = newOffset;
        start = newStart;
        end = newEnd;

        newLevel = level + 1;
        qint64 range = m_zoomer->traceDuration() >> newLevel;
        newOffset = (m_zoomer->windowStart() - m_zoomer->traceStart() + range / 2) / range;
        newStart = m_zoomer->traceStart() + newOffset * range - range / 2;
        newEnd = newStart + range;
    } while (newStart < m_zoomer->windowStart() && newEnd > m_zoomer->windowEnd());

    if (m_renderStates.length() <= level)
        m_renderStates.resize(level + 1);
    TimelineRenderState *state = m_renderStates[level][offset];
    if (state == nullptr) {
        state = new TimelineRenderState(start, end, 1.0 / static_cast<qreal>(SafeFloatMax),
                                        m_renderPasses.size());
        m_renderStates[level][offset] = state;
    }
    return state;
}

QSGNode *TimelineRenderer::updatePaintNode(QSGNode *node, UpdatePaintNodeData *updatePaintNodeData)
{
    if (!m_model || m_model->hidden() || m_model->isEmpty() || !m_zoomer ||
            m_zoomer->windowDuration() <= 0) {
        delete node;
        return nullptr;
    }

    float spacing = static_cast<float>(width() / m_zoomer->windowDuration());

    if (m_modelDirty) {
        if (node)
            node->removeAllChildNodes();
        clear();
    }

    TimelineRenderState *state = findRenderState();

    int lastIndex = m_model->lastIndex(m_zoomer->windowEnd());
    int firstIndex = m_model->firstIndex(m_zoomer->windowStart());

    for (int i = 0; i < m_renderPasses.length(); ++i)
        state->passes[i] = m_renderPasses[i]->update(this, state, state->passes[i],
                                                         firstIndex, lastIndex + 1,
                                                         state != m_lastState, spacing);

    if (state->isEmpty()) { // new state
        state->assembleNodeTree(m_model, TimelineModel::defaultRowHeight(),
                                TimelineModel::defaultRowHeight());
    } else if (m_rowHeightsDirty || state != m_lastState) {
        state->updateExpandedRowHeights(m_model, TimelineModel::defaultRowHeight(),
                                        TimelineModel::defaultRowHeight());
    }

    TimelineAbstractRenderer::updatePaintNode(nullptr, updatePaintNodeData);
    m_lastState = state;

    QMatrix4x4 matrix;
    matrix.translate((state->start - m_zoomer->windowStart()) * spacing, 0, 0);
    matrix.scale(spacing / state->scale, 1, 1);

    return state->finalize(node, m_model->expanded(), matrix);
}

void TimelineRenderer::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
}

int TimelineRenderer::rowFromPosition(int y) const
{
    if (!m_model->expanded())
        return y / TimelineModel::defaultRowHeight();

    int ret = 0;
    for (int row = 0; row < m_model->expandedRowCount(); ++row) {
        y -= m_model->expandedRowHeight(row);
        if (y <= 0) return ret;
        ++ret;
    }

    return ret;
}

void TimelineRenderer::mouseReleaseEvent(QMouseEvent *event)
{
    findCurrentSelection(event->pos().x(), event->pos().y(), width());
    setSelectedItem(m_currentEventIndex);
}

void TimelineRenderer::mouseMoveEvent(QMouseEvent *event)
{
    event->setAccepted(false);
}

void TimelineRenderer::hoverMoveEvent(QHoverEvent *event)
{
    if (!m_selectionLocked) {
        const QPoint pos = event->position().toPoint();
        findCurrentSelection(pos.x(), pos.y(), width());
        if (m_currentEventIndex != -1)
            setSelectedItem(m_currentEventIndex);
    }
    if (m_currentEventIndex == -1)
        event->setAccepted(false);
}

void TimelineRenderer::wheelEvent(QWheelEvent *event)
{
    // ctrl-wheel means zoom
    if (event->modifiers() & Qt::ControlModifier) {
        event->setAccepted(true);
        if (event->angleDelta().y() == 0)
            return;
        TimelineZoomControl *zoom = zoomer();

        // Handle similar to text editor, but avoid floats.
        // angleDelta of 120 is considered a 10% change in zoom.
        const qint64 delta = event->angleDelta().y();
        const qint64 newDuration = qBound(zoom->minimumRangeLength(),
                                          zoom->rangeDuration() * 1200 / (1200 + delta),
                                          std::max(zoom->minimumRangeLength(),
                                                   zoom->traceDuration()));
        const qint64 mouseTime = event->position().toPoint().x() * zoom->windowDuration() / width()
                                 + zoom->windowStart();
        // Try to keep mouseTime where it was in relation to the shown range,
        // but keep within traceStart/End
        const qint64 newStart
            = qBound(zoom->traceStart(),
                     mouseTime
                         - newDuration * /*rest is mouse time position [0,1] in range:*/
                               (mouseTime - zoom->rangeStart()) / zoom->rangeDuration(),
                     std::max(zoom->traceStart(), zoom->traceEnd() - newDuration));
        zoom->setRange(newStart, newStart + newDuration);
    } else {
        TimelineAbstractRenderer::wheelEvent(event);
    }
}

TimelineRenderer::MatchResult TimelineRenderer::checkMatch(MatchParameters *params, int index,
                                                           qint64 itemStart, qint64 itemEnd)
{
    const qint64 offset = qAbs(itemEnd - params->exactTime) + qAbs(itemStart - params->exactTime);
    if (offset >= params->bestOffset)
        return NoMatch;

    params->bestOffset = offset;
    m_currentEventIndex = index;

    return (itemEnd >= params->exactTime && itemStart <= params->exactTime)
            ? ExactMatch : ApproximateMatch;
}

TimelineRenderer::MatchResult TimelineRenderer::matchForward(MatchParameters *params, int index)
{
    if (index < 0)
        return NoMatch;

    if (index >= m_model->count())
        return Cutoff;

    if (m_model->row(index) != m_currentRow)
        return NoMatch;

    const qint64 itemEnd = m_model->endTime(index);
    if (itemEnd < params->startTime)
        return NoMatch;

    const qint64 itemStart = m_model->startTime(index);
    if (itemStart > params->endTime)
        return Cutoff;

    // Further iteration will only increase the startOffset.
    if (itemStart - params->exactTime >= params->bestOffset)
        return Cutoff;

    return checkMatch(params, index, itemStart, itemEnd);
}

TimelineRenderer::MatchResult TimelineRenderer::matchBackward(MatchParameters *params, int index)
{
    if (index < 0)
        return Cutoff;

    if (index >= m_model->count())
        return NoMatch;

    if (m_model->row(index) != m_currentRow)
        return NoMatch;

    const qint64 itemStart = m_model->startTime(index);
    if (itemStart > params->endTime)
        return NoMatch;

    // There can be small events that don't reach the cursor position after large events
    // that do but are in a different row. In that case, the parent index will be valid and will
    // point to the large event. If that is also outside the range, we are really done.
    const qint64 itemEnd = m_model->endTime(index);
    if (itemEnd < params->startTime) {
        const int parentIndex = m_model->parentIndex(index);
        const qint64 parentEnd = parentIndex == -1 ? itemEnd : m_model->endTime(parentIndex);
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

void TimelineRenderer::findCurrentSelection(int mouseX, int mouseY, int width)
{
    if (!m_zoomer || !m_model || width < 1)
        return;

    qint64 duration = m_zoomer->windowDuration();
    if (duration <= 0)
        return;

    MatchParameters params;

    // Make the "selected" area 3 pixels wide by adding/subtracting 1 to catch very narrow events.
    params.startTime = (mouseX - 1) * duration / width + m_zoomer->windowStart();
    params.endTime = (mouseX + 1) * duration / width + m_zoomer->windowStart();
    params.exactTime = (params.startTime + params.endTime) / 2;
    const int row = rowFromPosition(mouseY);

    // already covered? Only make sure m_selectedItem is correct.
    if (m_currentEventIndex != -1 &&
            params.exactTime >= m_model->startTime(m_currentEventIndex) &&
            params.exactTime < m_model->endTime(m_currentEventIndex) &&
            row == m_currentRow) {
        return;
    }

    m_currentRow = row;
    m_currentEventIndex = -1;

    const int middle = m_model->bestIndex(params.exactTime);
    if (middle == -1)
        return;

    params.bestOffset = std::numeric_limits<qint64>::max();
    const qint64 itemStart = m_model->startTime(middle);
    const qint64 itemEnd = m_model->endTime(middle);
    if (m_model->row(middle) == row && itemEnd >= params.startTime && itemStart <= params.endTime) {
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
    resetCurrentSelection();
    setSelectedItem(-1);
    setSelectionLocked(true);
}

void TimelineRenderer::selectNextFromSelectionId(int selectionId)
{
    setSelectedItem(m_model->nextItemBySelectionId(selectionId, m_zoomer->rangeStart(),
                                                   m_selectedItem));
}

void TimelineRenderer::selectPrevFromSelectionId(int selectionId)
{
    setSelectedItem(m_model->prevItemBySelectionId(selectionId, m_zoomer->rangeStart(),
                                                   m_selectedItem));
}

} // namespace Timeline
