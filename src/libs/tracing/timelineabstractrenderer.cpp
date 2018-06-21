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

#include "timelineabstractrenderer_p.h"

namespace Timeline {

TimelineAbstractRenderer::TimelineAbstractRendererPrivate::TimelineAbstractRendererPrivate() :
    selectedItem(-1), selectionLocked(true), model(0), notes(0), zoomer(0), modelDirty(false),
    rowHeightsDirty(false), notesDirty(false)
{
}

TimelineAbstractRenderer::TimelineAbstractRendererPrivate::~TimelineAbstractRendererPrivate()
{
    // Nothing to delete here as all the pointer members are owned by other classes.
}

TimelineAbstractRenderer::TimelineAbstractRenderer(TimelineAbstractRendererPrivate &dd,
                                                   QQuickItem *parent) :
    QQuickItem(parent), d_ptr(&dd)
{
    setFlag(ItemHasContents);
}

int TimelineAbstractRenderer::selectedItem() const
{
    Q_D(const TimelineAbstractRenderer);
    return d->selectedItem;
}

void TimelineAbstractRenderer::setSelectedItem(int itemIndex)
{
    Q_D(TimelineAbstractRenderer);
    if (d->selectedItem != itemIndex) {
        d->selectedItem = itemIndex;
        update();
        emit selectedItemChanged(itemIndex);
    }
}

TimelineAbstractRenderer::TimelineAbstractRenderer(QQuickItem *parent) : QQuickItem(parent),
    d_ptr(new TimelineAbstractRendererPrivate)
{
    setFlag(ItemHasContents);
}

TimelineAbstractRenderer::~TimelineAbstractRenderer()
{
    Q_D(TimelineAbstractRenderer);
    delete d;
}

bool TimelineAbstractRenderer::selectionLocked() const
{
    Q_D(const TimelineAbstractRenderer);
    return d->selectionLocked;
}

void TimelineAbstractRenderer::setSelectionLocked(bool locked)
{
    Q_D(TimelineAbstractRenderer);
    if (d->selectionLocked != locked) {
        d->selectionLocked = locked;
        update();
        emit selectionLockedChanged(locked);
    }
}

TimelineModel *TimelineAbstractRenderer::model() const
{
    Q_D(const TimelineAbstractRenderer);
    return d->model;
}

void TimelineAbstractRenderer::setModel(TimelineModel *model)
{
    Q_D(TimelineAbstractRenderer);
    if (d->model == model)
        return;

    if (d->model) {
        disconnect(d->model, &TimelineModel::expandedChanged, this, &QQuickItem::update);
        disconnect(d->model, &TimelineModel::hiddenChanged, this, &QQuickItem::update);
        disconnect(d->model, &TimelineModel::expandedRowHeightChanged,
                   this, &TimelineAbstractRenderer::setRowHeightsDirty);
        disconnect(d->model, &TimelineModel::contentChanged,
                   this, &TimelineAbstractRenderer::setModelDirty);
    }

    d->model = model;
    if (d->model) {
        connect(d->model, &TimelineModel::expandedChanged, this, &QQuickItem::update);
        connect(d->model, &TimelineModel::hiddenChanged, this, &QQuickItem::update);
        connect(d->model, &TimelineModel::expandedRowHeightChanged,
                this, &TimelineAbstractRenderer::setRowHeightsDirty);
        connect(d->model, &TimelineModel::contentChanged,
                this, &TimelineAbstractRenderer::setModelDirty);
        d->renderPasses = d->model->supportedRenderPasses();
    }

    setModelDirty();
    emit modelChanged(d->model);
}

TimelineNotesModel *TimelineAbstractRenderer::notes() const
{
    Q_D(const TimelineAbstractRenderer);
    return d->notes;
}

void TimelineAbstractRenderer::setNotes(TimelineNotesModel *notes)
{
    Q_D(TimelineAbstractRenderer);
    if (d->notes == notes)
        return;

    if (d->notes)
        disconnect(d->notes, &TimelineNotesModel::changed,
                   this, &TimelineAbstractRenderer::setNotesDirty);

    d->notes = notes;
    if (d->notes)
        connect(d->notes, &TimelineNotesModel::changed,
                this, &TimelineAbstractRenderer::setNotesDirty);

    setNotesDirty();
    emit notesChanged(d->notes);
}

TimelineZoomControl *TimelineAbstractRenderer::zoomer() const
{
    Q_D(const TimelineAbstractRenderer);
    return d->zoomer;
}

void TimelineAbstractRenderer::setZoomer(TimelineZoomControl *zoomer)
{
    Q_D(TimelineAbstractRenderer);
    if (zoomer != d->zoomer) {
        if (d->zoomer != 0)
            disconnect(d->zoomer, &TimelineZoomControl::windowChanged, this, &QQuickItem::update);
        d->zoomer = zoomer;
        if (d->zoomer != 0)
            connect(d->zoomer, &TimelineZoomControl::windowChanged, this, &QQuickItem::update);
        emit zoomerChanged(zoomer);
        update();
    }
}

bool TimelineAbstractRenderer::modelDirty() const
{
    Q_D(const TimelineAbstractRenderer);
    return d->modelDirty;
}

bool TimelineAbstractRenderer::notesDirty() const
{
    Q_D(const TimelineAbstractRenderer);
    return d->notesDirty;
}

bool TimelineAbstractRenderer::rowHeightsDirty() const
{
    Q_D(const TimelineAbstractRenderer);
    return d->rowHeightsDirty;
}

void TimelineAbstractRenderer::setModelDirty()
{
    Q_D(TimelineAbstractRenderer);
    d->modelDirty = true;
    update();
}

void TimelineAbstractRenderer::setRowHeightsDirty()
{
    Q_D(TimelineAbstractRenderer);
    d->rowHeightsDirty = true;
    update();
}

void TimelineAbstractRenderer::setNotesDirty()
{
    Q_D(TimelineAbstractRenderer);
    d->notesDirty = true;
    update();
}

// Reset the dirty flags, delete the old node (if given), and return 0
QSGNode *TimelineAbstractRenderer::updatePaintNode(QSGNode *oldNode,
                                                   UpdatePaintNodeData *updatePaintNodeData)
{
    Q_D(TimelineAbstractRenderer);
    d->modelDirty = false;
    d->rowHeightsDirty = false;
    d->notesDirty = false;
    return QQuickItem::updatePaintNode(oldNode, updatePaintNodeData);
}

} // namespace Timeline
