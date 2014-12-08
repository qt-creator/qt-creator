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

#include "timelinerenderer.h"
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

using namespace QmlProfiler;
using namespace QmlProfiler::Internal;

TimelineRenderer::TimelineRenderer(QQuickItem *parent) :
    QQuickItem(parent), m_model(0), m_zoomer(0), m_notes(0),
    m_selectedItem(-1), m_selectionLocked(true), m_modelDirty(false),
    m_rowHeightsDirty(false), m_rowCountsDirty(false), m_lastState(0)
{
    setFlag(QQuickItem::ItemHasContents);
    resetCurrentSelection();
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
}

void TimelineRenderer::setModel(TimelineModel *model)
{
    if (m_model == model)
        return;

    if (m_model) {
        disconnect(m_model, SIGNAL(expandedChanged()), this, SLOT(update()));
        disconnect(m_model, SIGNAL(hiddenChanged()), this, SLOT(update()));
        disconnect(m_model, SIGNAL(expandedRowHeightChanged(int,int)),
                   this, SLOT(setRowHeightsDirty()));
        disconnect(m_model, SIGNAL(emptyChanged()), this, SLOT(setModelDirty()));
        disconnect(m_model, SIGNAL(expandedRowCountChanged()), this, SLOT(setRowCountsDirty()));
        disconnect(m_model, SIGNAL(collapsedRowCountChanged()), this, SLOT(setRowCountsDirty()));
    }

    m_model = model;
    if (m_model) {
        connect(m_model, SIGNAL(expandedChanged()), this, SLOT(update()));
        connect(m_model, SIGNAL(hiddenChanged()), this, SLOT(update()));
        connect(m_model, SIGNAL(expandedRowHeightChanged(int,int)),
                this, SLOT(setRowHeightsDirty()));
        connect(m_model, SIGNAL(emptyChanged()), this, SLOT(setModelDirty()));
        connect(m_model, SIGNAL(expandedRowCountChanged()), this, SLOT(setRowCountsDirty()));
        connect(m_model, SIGNAL(collapsedRowCountChanged()), this, SLOT(setRowCountsDirty()));
        m_renderPasses = model->supportedRenderPasses();
    }

    setModelDirty();
    setRowHeightsDirty();
    setRowCountsDirty();
    emit modelChanged(m_model);
}

void TimelineRenderer::setZoomer(TimelineZoomControl *zoomer)
{
    if (zoomer != m_zoomer) {
        if (m_zoomer != 0)
            disconnect(m_zoomer, SIGNAL(windowChanged(qint64,qint64)), this, SLOT(update()));
        m_zoomer = zoomer;
        if (m_zoomer != 0)
            connect(m_zoomer, SIGNAL(windowChanged(qint64,qint64)), this, SLOT(update()));
        emit zoomerChanged(zoomer);
        update();
    }
}

void TimelineRenderer::setNotes(TimelineNotesModel *notes)
{
    if (m_notes == notes)
        return;

    if (m_notes)
        disconnect(m_notes, &TimelineNotesModel::changed,
                   this, &TimelineRenderer::setNotesDirty);

    m_notes = notes;
    if (m_notes)
        connect(m_notes, &TimelineNotesModel::changed,
                this, &TimelineRenderer::setNotesDirty);

    emit notesChanged(m_notes);
    update();
}

bool TimelineRenderer::modelDirty() const
{
    return m_modelDirty;
}

bool TimelineRenderer::notesDirty() const
{
    return m_notesDirty;
}

bool TimelineRenderer::rowHeightsDirty() const
{
    return m_rowHeightsDirty;
}

void TimelineRenderer::resetCurrentSelection()
{
    m_currentSelection.startTime = -1;
    m_currentSelection.endTime = -1;
    m_currentSelection.row = -1;
    m_currentSelection.eventIndex = -1;
}

TimelineRenderState *TimelineRenderer::findRenderState()
{
    int newLevel = 0;
    int newOffset = 0;
    int level;
    int offset;

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
    if (m_renderStates[level].length() <= offset)
        m_renderStates[level].resize(offset + 1);
    TimelineRenderState *state = m_renderStates[level][offset];
    if (state == 0) {
        state = new TimelineRenderState(start, end, 1.0 / static_cast<qreal>(SafeFloatMax),
                                        m_renderPasses.size());
        m_renderStates[level][offset] = state;
    }
    return state;
}

QSGNode *TimelineRenderer::updatePaintNode(QSGNode *node,
                                           UpdatePaintNodeData *updatePaintNodeData)
{
    Q_UNUSED(updatePaintNodeData)

    if (!m_model || m_model->hidden() || m_model->isEmpty() || m_zoomer->windowDuration() <= 0) {
        delete node;
        return 0;
    } else if (node == 0) {
        node = new QSGTransformNode;
    }

    qreal spacing = width() / m_zoomer->windowDuration();

    if (m_modelDirty || m_rowCountsDirty) {
        node->removeAllChildNodes();
        foreach (QVector<TimelineRenderState *> stateVector, m_renderStates)
            qDeleteAll(stateVector);
        m_renderStates.clear();
        m_lastState = 0;
    }

    TimelineRenderState *state = findRenderState();

    int lastIndex = m_model->lastIndex(m_zoomer->windowEnd());
    int firstIndex = m_model->firstIndex(m_zoomer->windowStart());

    for (int i = 0; i < m_renderPasses.length(); ++i)
        state->setPassState(i, m_renderPasses[i]->update(this, state, state->passState(i),
                                                         firstIndex, lastIndex + 1,
                                                         state != m_lastState, spacing));

    if (state->isEmpty()) { // new state
        for (int pass = 0; pass < m_renderPasses.length(); ++pass) {
            const TimelineRenderPass::State *passState = state->passState(pass);
            if (!passState)
                continue;
            if (passState->expandedOverlay)
                state->expandedOverlayRoot()->appendChildNode(passState->expandedOverlay);
            if (passState->collapsedOverlay)
                state->collapsedOverlayRoot()->appendChildNode(passState->collapsedOverlay);
        }

        int row = 0;
        for (int i = 0; i < m_model->expandedRowCount(); ++i) {
            QSGTransformNode *rowNode = new QSGTransformNode;
            for (int pass = 0; pass < m_renderPasses.length(); ++pass) {
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

        for (int row = 0; row < m_model->collapsedRowCount(); ++row) {
            QSGTransformNode *rowNode = new QSGTransformNode;
            QMatrix4x4 matrix;
            matrix.translate(0, row * TimelineModel::defaultRowHeight(), 0);
            rowNode->setMatrix(matrix);
            for (int pass = 0; pass < m_renderPasses.length(); ++pass) {
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

    if (m_rowHeightsDirty || state != m_lastState) {
        int row = 0;
        qreal offset = 0;
        for (QSGNode *rowNode = state->expandedRowRoot()->firstChild(); rowNode != 0;
             rowNode = rowNode->nextSibling()) {
            qreal rowHeight = m_model->expandedRowHeight(row++);
            QMatrix4x4 matrix;
            matrix.translate(0, offset, 0);
            matrix.scale(1, rowHeight / TimelineModel::defaultRowHeight(), 1);
            offset += rowHeight;
            static_cast<QSGTransformNode *>(rowNode)->setMatrix(matrix);
        }
    }

    m_modelDirty = false;
    m_notesDirty = false;
    m_rowCountsDirty = false;
    m_rowHeightsDirty = false;
    m_lastState = state;

    QSGNode *rowNode = m_model->expanded() ? state->expandedRowRoot() : state->collapsedRowRoot();
    QSGNode *overlayNode = m_model->expanded() ? state->expandedOverlayRoot() :
                                                 state->collapsedOverlayRoot();

    QMatrix4x4 matrix;
    matrix.translate((state->start() - m_zoomer->windowStart()) * spacing, 0, 0);
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

int TimelineRenderer::rowFromPosition(int y)
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
    Q_UNUSED(event);
    if (!m_model->isEmpty())
        manageClicked();
}

void TimelineRenderer::mouseMoveEvent(QMouseEvent *event)
{
    event->setAccepted(false);
}


void TimelineRenderer::hoverMoveEvent(QHoverEvent *event)
{
    Q_UNUSED(event);
    manageHovered(event->pos().x(), event->pos().y());
    if (m_currentSelection.eventIndex == -1)
        event->setAccepted(false);
}

void TimelineRenderer::manageClicked()
{
    if (m_currentSelection.eventIndex != -1) {
        if (m_currentSelection.eventIndex == m_selectedItem)
            setSelectionLocked(!m_selectionLocked);
        else
            setSelectionLocked(true);

        // itemPressed() will trigger an update of the events and JavaScript views. Make sure the
        // correct event is already selected when that happens, to prevent confusion.
        setSelectedItem(m_currentSelection.eventIndex);
        emit itemPressed(m_currentSelection.eventIndex);
    } else {
        setSelectionLocked(false);
        setSelectedItem(-1);
        emit itemPressed(-1);
    }
}

void TimelineRenderer::manageHovered(int mouseX, int mouseY)
{
    qint64 duration = m_zoomer->windowDuration();
    if (duration <= 0)
        return;

    // Make the "selected" area 3 pixels wide by adding/subtracting 1 to catch very narrow events.
    qint64 startTime = (mouseX - 1) * duration / width() + m_zoomer->windowStart();
    qint64 endTime = (mouseX + 1) * duration / width() + m_zoomer->windowStart();
    qint64 exactTime = (startTime + endTime) / 2;
    int row = rowFromPosition(mouseY);

    // already covered? Only recheck selectionLocked and make sure m_selectedItem is correct.
    if (m_currentSelection.eventIndex != -1 &&
            exactTime >= m_currentSelection.startTime &&
            exactTime < m_currentSelection.endTime &&
            row == m_currentSelection.row) {
        if (!m_selectionLocked)
            setSelectedItem(m_currentSelection.eventIndex);
        return;
    }

    // find if there's items in the time range
    int eventFrom = m_model->firstIndex(startTime);
    int eventTo = m_model->lastIndex(endTime);

    m_currentSelection.eventIndex = -1;
    if (eventFrom == -1 || eventTo < eventFrom || eventTo >= m_model->count())
        return;

    // find if we are in the right column
    qint64 bestOffset = std::numeric_limits<qint64>::max();
    for (int i=eventTo; i>=eventFrom; --i) {
        if ( m_model->row(i) == row) {
            // There can be small events that don't reach the cursor position after large events
            // that do but are in a different row.
            qint64 itemEnd = m_model->endTime(i);
            if (itemEnd < startTime)
                continue;

            qint64 itemStart = m_model->startTime(i);

            qint64 offset = qAbs(itemEnd - exactTime) + qAbs(itemStart - exactTime);
            if (offset < bestOffset) {
                // match
                m_currentSelection.eventIndex = i;
                m_currentSelection.startTime = itemStart;
                m_currentSelection.endTime = itemEnd;
                m_currentSelection.row = row;
                bestOffset = offset;
            }
        }
    }
    if (!m_selectionLocked && m_currentSelection.eventIndex != -1)
        setSelectedItem(m_currentSelection.eventIndex);
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

void TimelineRenderer::setModelDirty()
{
    m_modelDirty = true;
    update();
}

void TimelineRenderer::setRowHeightsDirty()
{
    m_rowHeightsDirty = true;
    update();
}

void TimelineRenderer::setNotesDirty()
{
    m_notesDirty = true;
    update();
}

void TimelineRenderer::setRowCountsDirty()
{
    m_rowCountsDirty = true;
}

