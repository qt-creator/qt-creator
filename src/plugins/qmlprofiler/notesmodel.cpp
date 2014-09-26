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
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "notesmodel.h"

namespace QmlProfiler {

NotesModel::NotesModel(QObject *parent) : QObject(parent), m_modelManager(0)
{
}

int NotesModel::count() const
{
    return m_data.count();
}

void NotesModel::setModelManager(QmlProfilerModelManager *modelManager)
{
    m_modelManager = modelManager;
}

void NotesModel::addTimelineModel(const AbstractTimelineModel *timelineModel)
{
    connect(timelineModel, &AbstractTimelineModel::destroyed,
            this, &NotesModel::removeTimelineModel);
    m_timelineModels.insert(timelineModel->modelId(), timelineModel);
}

int NotesModel::typeId(int index) const
{
    const Note &note = m_data[index];
    auto it = m_timelineModels.find(note.timelineModel);
    if (it == m_timelineModels.end())
        return -1; // This can happen if one of the timeline models has been removed
    return it.value()->typeId(note.timelineIndex);
}

QString NotesModel::text(int index) const
{
    return m_data[index].text;
}

int NotesModel::timelineModel(int index) const
{
    return m_data[index].timelineModel;
}

int NotesModel::timelineIndex(int index) const
{
    return m_data[index].timelineIndex;
}

QVariantList NotesModel::byTypeId(int selectedType) const
{
    QVariantList ret;
    for (int noteId = 0; noteId < count(); ++noteId) {
        if (selectedType == typeId(noteId))
            ret << noteId;
    }
    return ret;
}

QVariantList NotesModel::byTimelineModel(int timelineModel) const
{
    QVariantList ret;
    for (int noteId = 0; noteId < count(); ++noteId) {
        if (m_data[noteId].timelineModel == timelineModel)
            ret << noteId;
    }
    return ret;
}

int NotesModel::get(int timelineModel, int timelineIndex) const
{
    for (int noteId = 0; noteId < count(); ++noteId) {
        const Note &note = m_data[noteId];
        if (note.timelineModel == timelineModel && note.timelineIndex == timelineIndex)
            return noteId;
    }

    return -1;
}

int NotesModel::add(int timelineModel, int timelineIndex, const QString &text)
{
    const AbstractTimelineModel *model = m_timelineModels[timelineModel];
    int typeId = model->range(timelineIndex).typeId;
    Note note = { text, timelineModel, timelineIndex };
    m_data << note;
    emit changed(typeId, timelineModel, timelineIndex);
    return m_data.count() - 1;
}

void NotesModel::update(int index, const QString &text)
{
    Note &note = m_data[index];
    if (text != note.text) {
        note.text = text;
        emit changed(typeId(index), note.timelineModel, note.timelineIndex);
    }
}

void NotesModel::remove(int index)
{
    Note &note = m_data[index];
    int noteType = typeId(index);
    int timelineModel = note.timelineModel;
    int timelineIndex = note.timelineIndex;
    m_data.removeAt(index);
    emit changed(noteType, timelineModel, timelineIndex);
}

void NotesModel::removeTimelineModel(QObject *timelineModel)
{
    for (auto i = m_timelineModels.begin(); i != m_timelineModels.end();) {
        if (i.value() == timelineModel)
            i = m_timelineModels.erase(i);
        else
            ++i;
    }
}

int NotesModel::add(int typeId, qint64 start, qint64 duration, const QString &text)
{
    int timelineModel = -1;
    int timelineIndex = -1;
    const QVector<QmlProfilerDataModel::QmlEventTypeData> &types =
            m_modelManager->qmlModel()->getEventTypes();
    foreach (const AbstractTimelineModel *model, m_timelineModels) {
        if (model->accepted(types[typeId])) {
            for (int i = model->firstIndex(start); i <= model->lastIndex(start + duration); ++i) {
                if (i < 0)
                    continue;
                const SortedTimelineModel::Range &timelineRange = model->range(i);
                if (timelineRange.typeId == typeId && timelineRange.start == start &&
                        timelineRange.duration == duration) {
                    timelineModel = model->modelId();
                    timelineIndex = i;
                    break;
                }
            }
            if (timelineIndex != -1)
                break;
        }
    }

    if (timelineModel == -1 || timelineIndex == -1)
        return -1;

    Note note = { text, timelineModel, timelineIndex };
    m_data << note;
    return m_data.count() - 1;
}

void NotesModel::clear()
{
    m_data.clear();
    emit changed(-1, -1, -1);
}

void NotesModel::loadData()
{
    m_data.clear();
    const QVector<QmlProfilerDataModel::QmlEventNoteData> &notes =
            m_modelManager->qmlModel()->getEventNotes();
    for (int i = 0; i != notes.size(); ++i) {
        const QmlProfilerDataModel::QmlEventNoteData &note = notes[i];
        add(note.typeIndex, note.startTime, note.duration, note.text);
    }
    emit changed(-1, -1, -1);
}

void NotesModel::saveData()
{
    QVector<QmlProfilerDataModel::QmlEventNoteData> notes;
    for (int i = 0; i < count(); ++i) {
        const Note &note = m_data[i];
        auto it = m_timelineModels.find(note.timelineModel);
        if (it == m_timelineModels.end())
            continue;

        const SortedTimelineModel::Range &noteRange = it.value()->range(note.timelineIndex);
        QmlProfilerDataModel::QmlEventNoteData save = {
            noteRange.typeId, noteRange.start, noteRange.duration, note.text
        };
        notes.append(save);
    }
    m_modelManager->qmlModel()->setNoteData(notes);
}
}
