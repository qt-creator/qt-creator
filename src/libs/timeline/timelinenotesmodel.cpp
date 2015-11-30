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

#include "timelinenotesmodel_p.h"

namespace Timeline {

TimelineNotesModel::TimelineNotesModelPrivate::TimelineNotesModelPrivate(TimelineNotesModel *q) :
    modified(false), q_ptr(q)
{
}

TimelineNotesModel::TimelineNotesModel(QObject *parent) : QObject(parent),
    d_ptr(new TimelineNotesModelPrivate(this))
{
}

TimelineNotesModel::~TimelineNotesModel()
{
    Q_D(TimelineNotesModel);
    delete d;
}

int TimelineNotesModel::count() const
{
    Q_D(const TimelineNotesModel);
    return d->data.count();
}

void TimelineNotesModel::addTimelineModel(const TimelineModel *timelineModel)
{
    Q_D(TimelineNotesModel);
    connect(timelineModel, SIGNAL(destroyed(QObject*)),
            this, SLOT(_q_removeTimelineModel(QObject*)));
    d->timelineModels.insert(timelineModel->modelId(), timelineModel);
}

const TimelineModel *TimelineNotesModel::timelineModelByModelId(int modelId) const
{
    Q_D(const TimelineNotesModel);
    auto it = d->timelineModels.find(modelId);
    return it == d->timelineModels.end() ? 0 : it.value();
}

QList<const TimelineModel *> TimelineNotesModel::timelineModels() const
{
    Q_D(const TimelineNotesModel);
    return d->timelineModels.values();
}

int TimelineNotesModel::typeId(int index) const
{
    Q_D(const TimelineNotesModel);
    const TimelineNotesModelPrivate::Note &note = d->data[index];
    const TimelineModel *model = timelineModelByModelId(note.timelineModel);
    if (!model)
        return -1; // This can happen if one of the timeline models has been removed
    return model->typeId(note.timelineIndex);
}

QString TimelineNotesModel::text(int index) const
{
    Q_D(const TimelineNotesModel);
    return d->data[index].text;
}

int TimelineNotesModel::timelineModel(int index) const
{
    Q_D(const TimelineNotesModel);
    return d->data[index].timelineModel;
}

int TimelineNotesModel::timelineIndex(int index) const
{
    Q_D(const TimelineNotesModel);
    return d->data[index].timelineIndex;
}

QVariantList TimelineNotesModel::byTypeId(int selectedType) const
{
    QVariantList ret;
    for (int noteId = 0; noteId < count(); ++noteId) {
        if (selectedType == typeId(noteId))
            ret << noteId;
    }
    return ret;
}

QVariantList TimelineNotesModel::byTimelineModel(int modelId) const
{
    Q_D(const TimelineNotesModel);
    QVariantList ret;
    for (int noteId = 0; noteId < count(); ++noteId) {
        if (d->data[noteId].timelineModel == modelId)
            ret << noteId;
    }
    return ret;
}

int TimelineNotesModel::get(int modelId, int timelineIndex) const
{
    Q_D(const TimelineNotesModel);
    for (int noteId = 0; noteId < count(); ++noteId) {
        const TimelineNotesModelPrivate::Note &note = d->data[noteId];
        if (note.timelineModel == modelId && note.timelineIndex == timelineIndex)
            return noteId;
    }

    return -1;
}

int TimelineNotesModel::add(int modelId, int timelineIndex, const QString &text)
{
    Q_D(TimelineNotesModel);
    const TimelineModel *model = d->timelineModels[modelId];
    int typeId = model->typeId(timelineIndex);
    TimelineNotesModelPrivate::Note note = { text, modelId, timelineIndex };
    d->data << note;
    d->modified = true;
    emit changed(typeId, modelId, timelineIndex);
    return d->data.count() - 1;
}

void TimelineNotesModel::update(int index, const QString &text)
{
    Q_D(TimelineNotesModel);
    TimelineNotesModelPrivate::Note &note = d->data[index];
    if (text != note.text) {
        note.text = text;
        d->modified = true;
        emit changed(typeId(index), note.timelineModel, note.timelineIndex);
    }
}

void TimelineNotesModel::remove(int index)
{
    Q_D(TimelineNotesModel);
    TimelineNotesModelPrivate::Note &note = d->data[index];
    int noteType = typeId(index);
    int timelineModel = note.timelineModel;
    int timelineIndex = note.timelineIndex;
    d->data.removeAt(index);
    d->modified = true;
    emit changed(noteType, timelineModel, timelineIndex);
}

bool TimelineNotesModel::isModified() const
{
    Q_D(const TimelineNotesModel);
    return d->modified;
}

void TimelineNotesModel::resetModified()
{
    Q_D(TimelineNotesModel);
    d->modified = false;
}

void TimelineNotesModel::TimelineNotesModelPrivate::_q_removeTimelineModel(QObject *timelineModel)
{
    for (auto i = timelineModels.begin(); i != timelineModels.end();) {
        if (i.value() == timelineModel)
            i = timelineModels.erase(i);
        else
            ++i;
    }
}

void TimelineNotesModel::setText(int noteId, const QString &text)
{
    if (text.length() > 0)
        update(noteId, text);
    else
        remove(noteId);
}

void TimelineNotesModel::setText(int modelId, int index, const QString &text)
{
    int noteId = get(modelId, index);
    if (noteId == -1) {
        if (text.length() > 0)
            add(modelId, index, text);
    } else {
        setText(noteId, text);
    }
}

void TimelineNotesModel::clear()
{
    Q_D(TimelineNotesModel);
    d->data.clear();
    d->modified = false;
    emit changed(-1, -1, -1);
}

} // namespace Timeline

#include "moc_timelinenotesmodel.cpp"
