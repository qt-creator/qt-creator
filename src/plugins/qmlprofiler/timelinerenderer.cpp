/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "timelinerenderer_p.h"
#include "timelinerenderpass.h"
#include "qmlprofilernotesmodel.h"
#include "timelineitemsrenderpass.h"
#include "qmlprofilerbindingloopsrenderpass.h"
#include "timelineselectionrenderpass.h"
#include "timelinenotesrenderpass.h"

#include <QElapsedTimer>
#include <QQmlContext>
#include <QQmlProperty>
#include <QTimer>
#include <QPixmap>
#include <QGraphicsSceneMouseEvent>
#include <QVarLengthArray>
#include <QSGTransformNode>
#include <QSGSimpleRectNode>

#include <math.h>

namespace Timeline {

TimelineRenderer::TimelineRendererPrivate::TimelineRendererPrivate(TimelineRenderer *q) :
    model(0), zoomer(0), notes(0), selectedItem(-1), selectionLocked(true), modelDirty(false),
    rowHeightsDirty(false), rowCountsDirty(false), lastState(0), q_ptr(q)
{
    resetCurrentSelection();
}

TimelineRenderer::TimelineRenderer(QQuickItem *parent) :
    QQuickItem(parent), d_ptr(new TimelineRendererPrivate(this))
{
    setFlag(QQuickItem::ItemHasContents);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
}

bool TimelineRenderer::selectionLocked() const
{
    Q_D(const TimelineRenderer);
    return d->selectionLocked;
}

int TimelineRenderer::selectedItem() const
{
    Q_D(const TimelineRenderer);
    return d->selectedItem;
}

TimelineModel *TimelineRenderer::model() const
{
    Q_D(const TimelineRenderer);
    return d->model;
}

void TimelineRenderer::setModel(TimelineModel *model)
{
    Q_D(TimelineRenderer);
    if (d->model == model)
        return;

    if (d->model) {
        disconnect(d->model, SIGNAL(expandedChanged()), this, SLOT(update()));
        disconnect(d->model, SIGNAL(hiddenChanged()), this, SLOT(update()));
        disconnect(d->model, SIGNAL(expandedRowHeightChanged(int,int)),
                   this, SLOT(setRowHeightsDirty()));
        disconnect(d->model, SIGNAL(emptyChanged()), this, SLOT(setModelDirty()));
        disconnect(d->model, SIGNAL(expandedRowCountChanged()), this, SLOT(setRowCountsDirty()));
        disconnect(d->model, SIGNAL(collapsedRowCountChanged()), this, SLOT(setRowCountsDirty()));
    }

    d->model = model;
    if (d->model) {
        connect(d->model, SIGNAL(expandedChanged()), this, SLOT(update()));
        connect(d->model, SIGNAL(hiddenChanged()), this, SLOT(update()));
        connect(d->model, SIGNAL(expandedRowHeightChanged(int,int)),
                this, SLOT(setRowHeightsDirty()));
        connect(d->model, SIGNAL(emptyChanged()), this, SLOT(setModelDirty()));
        connect(d->model, SIGNAL(expandedRowCountChanged()), this, SLOT(setRowCountsDirty()));
        connect(d->model, SIGNAL(collapsedRowCountChanged()), this, SLOT(setRowCountsDirty()));
        d->renderPasses = d->model->supportedRenderPasses();
    }

    setModelDirty();
    setRowHeightsDirty();
    setRowCountsDirty();
    emit modelChanged(d->model);
}

TimelineZoomControl *TimelineRenderer::zoomer() const
{
    Q_D(const TimelineRenderer);
    return d->zoomer;
}

void TimelineRenderer::setZoomer(TimelineZoomControl *zoomer)
{
    Q_D(TimelineRenderer);
    if (zoomer != d->zoomer) {
        if (d->zoomer != 0)
            disconnect(d->zoomer, SIGNAL(windowChanged(qint64,qint64)), this, SLOT(update()));
        d->zoomer = zoomer;
        if (d->zoomer != 0)
            connect(d->zoomer, SIGNAL(windowChanged(qint64,qint64)), this, SLOT(update()));
        emit zoomerChanged(zoomer);
        update();
    }
}

TimelineNotesModel *TimelineRenderer::notes() const
{
    Q_D(const TimelineRenderer);
    return d->notes;
}

void TimelineRenderer::setNotes(TimelineNotesModel *notes)
{
    Q_D(TimelineRenderer);
    if (d->notes == notes)
        return;

    if (d->notes)
        disconnect(d->notes, &TimelineNotesModel::changed,
                   this, &TimelineRenderer::setNotesDirty);

    d->notes = notes;
    if (d->notes)
        connect(d->notes, &TimelineNotesModel::changed,
                this, &TimelineRenderer::setNotesDirty);

    emit notesChanged(d->notes);
    update();
}

bool TimelineRenderer::modelDirty() const
{
    Q_D(const TimelineRenderer);
    return d->modelDirty;
}

bool TimelineRenderer::notesDirty() const
{
    Q_D(const TimelineRenderer);
    return d->notesDirty;
}

bool TimelineRenderer::rowHeightsDirty() const
{
    Q_D(const TimelineRenderer);
    return d->rowHeightsDirty;
}

bool TimelineRenderer::rowCountsDirty() const
{
    Q_D(const TimelineRenderer);
    return d->rowCountsDirty;
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
    int newOffset = 0;
    int level;
    int offset;

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
    if (renderStates[level].length() <= offset)
        renderStates[level].resize(offset + 1);
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
    Q_UNUSED(updatePaintNodeData)

    if (!d->model || d->model->hidden() || d->model->isEmpty() ||
            d->zoomer->windowDuration() <= 0) {
        delete node;
        return 0;
    } else if (node == 0) {
        node = new QSGTransformNode;
    }

    qreal spacing = width() / d->zoomer->windowDuration();

    if (d->modelDirty || d->rowCountsDirty) {
        node->removeAllChildNodes();
        foreach (QVector<TimelineRenderState *> stateVector, d->renderStates)
            qDeleteAll(stateVector);
        d->renderStates.clear();
        d->lastState = 0;
    }

    TimelineRenderState *state = d->findRenderState();

    int lastIndex = d->model->lastIndex(d->zoomer->windowEnd());
    int firstIndex = d->model->firstIndex(d->zoomer->windowStart());

    for (int i = 0; i < d->renderPasses.length(); ++i)
        state->setPassState(i, d->renderPasses[i]->update(this, state, state->passState(i),
                                                         firstIndex, lastIndex + 1,
                                                         state != d->lastState, spacing));

    if (state->isEmpty()) { // new state
        for (int pass = 0; pass < d->renderPasses.length(); ++pass) {
            const TimelineRenderPass::State *passState = state->passState(pass);
            if (!passState)
                continue;
            if (passState->expandedOverlay)
                state->expandedOverlayRoot()->appendChildNode(passState->expandedOverlay);
            if (passState->collapsedOverlay)
                state->collapsedOverlayRoot()->appendChildNode(passState->collapsedOverlay);
        }

        int row = 0;
        for (int i = 0; i < d->model->expandedRowCount(); ++i) {
            QSGTransformNode *rowNode = new QSGTransformNode;
            for (int pass = 0; pass < d->renderPasses.length(); ++pass) {
                const TimelineRenderPass::State *passState = state->passState(pass);
                if (passState && passState->expandedRows.length() > row) {
                    QSGNode *rowChildNode = passState->expandedRows[row];
                    if (rowChildNode)
                        rowNode->appendChildNode(rowChildNode);
                }
            }
            state->expandedRowRoot()->appendChildNode(rowNode);
            ++row;
        }

        for (int row = 0; row < d->model->collapsedRowCount(); ++row) {
            QSGTransformNode *rowNode = new QSGTransformNode;
            QMatrix4x4 matrix;
            matrix.translate(0, row * TimelineModel::defaultRowHeight(), 0);
            rowNode->setMatrix(matrix);
            for (int pass = 0; pass < d->renderPasses.length(); ++pass) {
                const TimelineRenderPass::State *passState = state->passState(pass);
                if (passState && passState->collapsedRows.length() > row) {
                    QSGNode *rowChildNode = passState->collapsedRows[row];
                    if (rowChildNode)
                        rowNode->appendChildNode(rowChildNode);
                }
            }
            state->collapsedRowRoot()->appendChildNode(rowNode);
        }
    }

    if (d->rowHeightsDirty || state != d->lastState) {
        int row = 0;
        qreal offset = 0;
        for (QSGNode *rowNode = state->expandedRowRoot()->firstChild(); rowNode != 0;
             rowNode = rowNode->nextSibling()) {
            qreal rowHeight = d->model->expandedRowHeight(row++);
            QMatrix4x4 matrix;
            matrix.translate(0, offset, 0);
            matrix.scale(1, rowHeight / TimelineModel::defaultRowHeight(), 1);
            offset += rowHeight;
            static_cast<QSGTransformNode *>(rowNode)->setMatrix(matrix);
        }
    }

    d->modelDirty = false;
    d->notesDirty = false;
    d->rowCountsDirty = false;
    d->rowHeightsDirty = false;
    d->lastState = state;

    QSGNode *rowNode = d->model->expanded() ? state->expandedRowRoot() : state->collapsedRowRoot();
    QSGNode *overlayNode = d->model->expanded() ? state->expandedOverlayRoot() :
                                                 state->collapsedOverlayRoot();

    QMatrix4x4 matrix;
    matrix.translate((state->start() - d->zoomer->windowStart()) * spacing, 0, 0);
    matrix.scale(spacing / state->scale(), 1, 1);

    QSGTransformNode *transform = static_cast<QSGTransformNode *>(node);
    transform->setMatrix(matrix);

    if (node->firstChild() != rowNode || node->lastChild() != overlayNode) {
        node->removeAllChildNodes();
        node->appendChildNode(rowNode);
        node->appendChildNode(overlayNode);
    }
    return node;
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
    if (!d->model->isEmpty())
        d->manageClicked();
}

void TimelineRenderer::mouseMoveEvent(QMouseEvent *event)
{
    event->setAccepted(false);
}

void TimelineRenderer::hoverMoveEvent(QHoverEvent *event)
{
    Q_D(TimelineRenderer);
    Q_UNUSED(event);
    d->manageHovered(event->pos().x(), event->pos().y());
    if (d->currentSelection.eventIndex == -1)
        event->setAccepted(false);
}

void TimelineRenderer::TimelineRendererPrivate::manageClicked()
{
    Q_Q(TimelineRenderer);
    if (currentSelection.eventIndex != -1) {
        if (currentSelection.eventIndex == selectedItem)
            q->setSelectionLocked(!selectionLocked);
        else
            q->setSelectionLocked(true);

        // itemPressed() will trigger an update of the events and JavaScript views. Make sure the
        // correct event is already selected when that happens, to prevent confusion.
        q->setSelectedItem(currentSelection.eventIndex);
        emit q->itemPressed(currentSelection.eventIndex);
    } else {
        q->setSelectionLocked(false);
        q->setSelectedItem(-1);
        emit q->itemPressed(-1);
    }
}

void TimelineRenderer::TimelineRendererPrivate::manageHovered(int mouseX, int mouseY)
{
    Q_Q(TimelineRenderer);
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
        if (model->row(i) == row) {
            // There can be small events that don't reach the cursor position after large events
            // that do but are in a different row.
            qint64 itemEnd = model->endTime(i);
            if (itemEnd < startTime)
                continue;

            qint64 itemStart = model->startTime(i);

            qint64 offset = qAbs(itemEnd - exactTime) + qAbs(itemStart - exactTime);
            if (offset < bestOffset) {
                // match
                currentSelection.eventIndex = i;
                currentSelection.startTime = itemStart;
                currentSelection.endTime = itemEnd;
                currentSelection.row = row;
                bestOffset = offset;
            }
        }
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

void TimelineRenderer::setSelectedItem(int itemIndex)
{
    Q_D(TimelineRenderer);
    if (d->selectedItem != itemIndex) {
        d->selectedItem = itemIndex;
        update();
        emit selectedItemChanged(itemIndex);
    }
}

void TimelineRenderer::setSelectionLocked(bool locked)
{
    Q_D(TimelineRenderer);
    if (d->selectionLocked != locked) {
        d->selectionLocked = locked;
        update();
        emit selectionLockedChanged(locked);
    }
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

void TimelineRenderer::setModelDirty()
{
    Q_D(TimelineRenderer);
    d->modelDirty = true;
    update();
}

void TimelineRenderer::setRowHeightsDirty()
{
    Q_D(TimelineRenderer);
    d->rowHeightsDirty = true;
    update();
}

void TimelineRenderer::setNotesDirty()
{
    Q_D(TimelineRenderer);
    d->notesDirty = true;
    update();
}

void TimelineRenderer::setRowCountsDirty()
{
    Q_D(TimelineRenderer);
    d->rowCountsDirty = true;
    update();
}

} // namespace Timeline
