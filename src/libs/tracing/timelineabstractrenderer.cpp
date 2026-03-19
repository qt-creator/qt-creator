// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelineabstractrenderer.h"

namespace Timeline {

TimelineAbstractRenderer::TimelineAbstractRenderer(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents);
}

int TimelineAbstractRenderer::selectedItem() const
{
    return m_selectedItem;
}

void TimelineAbstractRenderer::setSelectedItem(int itemIndex)
{
    if (m_selectedItem != itemIndex) {
        m_selectedItem = itemIndex;
        update();
        emit selectedItemChanged(itemIndex);
    }
}

bool TimelineAbstractRenderer::selectionLocked() const
{
    return m_selectionLocked;
}

void TimelineAbstractRenderer::setSelectionLocked(bool locked)
{
    if (m_selectionLocked != locked) {
        m_selectionLocked = locked;
        update();
        emit selectionLockedChanged(locked);
    }
}

TimelineModel *TimelineAbstractRenderer::model() const
{
    return m_model;
}

void TimelineAbstractRenderer::setModel(TimelineModel *model)
{
    if (m_model == model)
        return;

    if (m_model) {
        disconnect(m_model, &TimelineModel::expandedChanged, this, &QQuickItem::update);
        disconnect(m_model, &TimelineModel::hiddenChanged, this, &QQuickItem::update);
        disconnect(m_model, &TimelineModel::expandedRowHeightChanged,
                   this, &TimelineAbstractRenderer::setRowHeightsDirty);
        disconnect(m_model, &TimelineModel::contentChanged,
                   this, &TimelineAbstractRenderer::setModelDirty);
        disconnect(m_model, &QObject::destroyed, this, nullptr);
        m_renderPasses.clear();
    }

    m_model = model;

    if (m_model) {
        connect(m_model, &TimelineModel::expandedChanged, this, &QQuickItem::update);
        connect(m_model, &TimelineModel::hiddenChanged, this, &QQuickItem::update);
        connect(m_model, &TimelineModel::expandedRowHeightChanged,
                this, &TimelineAbstractRenderer::setRowHeightsDirty);
        connect(m_model, &TimelineModel::contentChanged,
                this, &TimelineAbstractRenderer::setModelDirty);
        connect(m_model, &QObject::destroyed, this, [this] {
            // Weak pointers are supposed to be notified before the destroyed() signal is sent.
            Q_ASSERT(m_model.isNull());
            m_renderPasses.clear();
            setModelDirty();
            emit modelChanged(m_model);
        });
        m_renderPasses = m_model->supportedRenderPasses();
    }

    setModelDirty();
    emit modelChanged(m_model);
}

TimelineNotesModel *TimelineAbstractRenderer::notes() const
{
    return m_notes;
}

void TimelineAbstractRenderer::setNotes(TimelineNotesModel *notes)
{
    if (m_notes == notes)
        return;

    if (m_notes) {
        disconnect(m_notes, &TimelineNotesModel::changed,
                   this, &TimelineAbstractRenderer::setNotesDirty);
        disconnect(m_notes, &QObject::destroyed, this, nullptr);
    }

    m_notes = notes;
    if (m_notes) {
        connect(m_notes, &TimelineNotesModel::changed,
                this, &TimelineAbstractRenderer::setNotesDirty);
        connect(m_notes, &QObject::destroyed, this, [this] {
            // Weak pointers are supposed to be notified before the destroyed() signal is sent.
            Q_ASSERT(m_notes.isNull());
            setNotesDirty();
            emit notesChanged(m_notes);
        });
    }

    setNotesDirty();
    emit notesChanged(m_notes);
}

TimelineZoomControl *TimelineAbstractRenderer::zoomer() const
{
    return m_zoomer;
}

void TimelineAbstractRenderer::setZoomer(TimelineZoomControl *zoomer)
{
    if (zoomer != m_zoomer) {
        if (m_zoomer) {
            disconnect(m_zoomer, &TimelineZoomControl::windowChanged, this, &QQuickItem::update);
            disconnect(m_zoomer, &QObject::destroyed, this, nullptr);
        }
        m_zoomer = zoomer;
        if (m_zoomer) {
            connect(m_zoomer, &TimelineZoomControl::windowChanged, this, &QQuickItem::update);
            connect(m_zoomer, &QObject::destroyed, this, [this] {
                // Weak pointers are supposed to be notified before the destroyed() signal is sent.
                Q_ASSERT(m_zoomer.isNull());
                emit zoomerChanged(m_zoomer);
                update();
            });
        }
        emit zoomerChanged(zoomer);
        update();
    }
}

bool TimelineAbstractRenderer::modelDirty() const { return m_modelDirty; }
bool TimelineAbstractRenderer::notesDirty() const { return m_notesDirty; }
bool TimelineAbstractRenderer::rowHeightsDirty() const { return m_rowHeightsDirty; }

void TimelineAbstractRenderer::setModelDirty()
{
    if (!m_modelDirty) {
        m_modelDirty = true;
        update();
    }
}

void TimelineAbstractRenderer::setRowHeightsDirty()
{
    if (!m_rowHeightsDirty) {
        m_rowHeightsDirty = true;
        update();
    }
}

void TimelineAbstractRenderer::setNotesDirty()
{
    if (!m_notesDirty) {
        m_notesDirty = true;
        update();
    }
}

// Reset the dirty flags and return 0
QSGNode *TimelineAbstractRenderer::updatePaintNode(QSGNode *oldNode,
                                                   UpdatePaintNodeData *updatePaintNodeData)
{
    m_modelDirty = false;
    m_rowHeightsDirty = false;
    m_notesDirty = false;
    return QQuickItem::updatePaintNode(oldNode, updatePaintNodeData);
}

} // namespace Timeline
