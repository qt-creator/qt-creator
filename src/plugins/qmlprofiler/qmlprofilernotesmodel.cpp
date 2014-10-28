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

#include "qmlprofilernotesmodel.h"

namespace QmlProfiler {

QmlProfilerNotesModel::QmlProfilerNotesModel(QObject *parent) : QObject(parent), m_modelManager(0),
    m_modified(false)
{
}

int QmlProfilerNotesModel::count() const
{
    return m_data.count();
}

void QmlProfilerNotesModel::setModelManager(QmlProfilerModelManager *modelManager)
{
    m_modelManager = modelManager;
}

void QmlProfilerNotesModel::addTimelineModel(const QmlProfilerTimelineModel *timelineModel)
{
    connect(timelineModel, &QmlProfilerTimelineModel::destroyed,
            this, &QmlProfilerNotesModel::removeTimelineModel);
    m_timelineModels.insert(timelineModel->modelId(), timelineModel);
}

int QmlProfilerNotesModel::typeId(int index) const
{
    const Note &note = m_data[index];
    auto it = m_timelineModels.find(note.timelineModel);
    if (it == m_timelineModels.end())
        return -1; // This can happen if one of the timeline models has been removed
    return it.value()->typeId(note.timelineIndex);
}

QString QmlProfilerNotesModel::text(int index) const
{
    return m_data[index].text;
}

int QmlProfilerNotesModel::timelineModel(int index) const
{
    return m_data[index].timelineModel;
}

int QmlProfilerNotesModel::timelineIndex(int index) const
{
    return m_data[index].timelineIndex;
}

QVariantList QmlProfilerNotesModel::byTypeId(int selectedType) const
{
    QVariantList ret;
    for (int noteId = 0; noteId < count(); ++noteId) {
        if (selectedType == typeId(noteId))
            ret << noteId;
    }
    return ret;
}

QVariantList QmlProfilerNotesModel::byTimelineModel(int timelineModel) const
{
    QVariantList ret;
    for (int noteId = 0; noteId < count(); ++noteId) {
        if (m_data[noteId].timelineModel == timelineModel)
            ret << noteId;
    }
    return ret;
}

int QmlProfilerNotesModel::get(int timelineModel, int timelineIndex) const
{
    for (int noteId = 0; noteId < count(); ++noteId) {
        const Note &note = m_data[noteId];
        if (note.timelineModel == timelineModel && note.timelineIndex == timelineIndex)
            return noteId;
    }

    return -1;
}

int QmlProfilerNotesModel::add(int timelineModel, int timelineIndex, const QString &text)
{
    const QmlProfilerTimelineModel *model = m_timelineModels[timelineModel];
    int typeId = model->typeId(timelineIndex);
    Note note = { text, timelineModel, timelineIndex };
    m_data << note;
    m_modified = true;
    emit changed(typeId, timelineModel, timelineIndex);
    return m_data.count() - 1;
}

void QmlProfilerNotesModel::update(int index, const QString &text)
{
    Note &note = m_data[index];
    if (text != note.text) {
        note.text = text;
        m_modified = true;
        emit changed(typeId(index), note.timelineModel, note.timelineIndex);
    }
}

void QmlProfilerNotesModel::remove(int index)
{
    Note &note = m_data[index];
    int noteType = typeId(index);
    int timelineModel = note.timelineModel;
    int timelineIndex = note.timelineIndex;
    m_data.removeAt(index);
    m_modified = true;
    emit changed(noteType, timelineModel, timelineIndex);
}

bool QmlProfilerNotesModel::isModified() const
{
    return m_modified;
}

void QmlProfilerNotesModel::removeTimelineModel(QObject *timelineModel)
{
    for (auto i = m_timelineModels.begin(); i != m_timelineModels.end();) {
        if (i.value() == timelineModel)
            i = m_timelineModels.erase(i);
        else
            ++i;
    }
}

int QmlProfilerNotesModel::add(int typeId, qint64 start, qint64 duration, const QString &text)
{
    int timelineModel = -1;
    int timelineIndex = -1;
    const QVector<QmlProfilerDataModel::QmlEventTypeData> &types =
            m_modelManager->qmlModel()->getEventTypes();
    foreach (const QmlProfilerTimelineModel *model, m_timelineModels) {
        if (model->accepted(types[typeId])) {
            for (int i = model->firstIndex(start); i <= model->lastIndex(start + duration); ++i) {
                if (i < 0)
                    continue;
                if (model->typeId(i) == typeId && model->startTime(i) == start &&
                        model->duration(i) == duration) {
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
    m_modified = true;
    return m_data.count() - 1;
}

void QmlProfilerNotesModel::setText(int noteId, const QString &text)
{
    if (text.length() > 0) {
        update(noteId, text);
    } else {
        remove(noteId);
    }
}

void QmlProfilerNotesModel::setText(int modelIndex, int index, const QString &text)
{
    int noteId = get(modelIndex, index);
    if (noteId == -1) {
        if (text.length() > 0)
            add(modelIndex, index, text);
    } else {
        setText(noteId, text);
    }
}

void QmlProfilerNotesModel::clear()
{
    m_data.clear();
    m_modified = false;
    emit changed(-1, -1, -1);
}

void QmlProfilerNotesModel::loadData()
{
    m_data.clear();
    const QVector<QmlProfilerDataModel::QmlEventNoteData> &notes =
            m_modelManager->qmlModel()->getEventNotes();
    for (int i = 0; i != notes.size(); ++i) {
        const QmlProfilerDataModel::QmlEventNoteData &note = notes[i];
        add(note.typeIndex, note.startTime, note.duration, note.text);
    }
    m_modified = false; // reset after loading
    emit changed(-1, -1, -1);
}

void QmlProfilerNotesModel::saveData()
{
    QVector<QmlProfilerDataModel::QmlEventNoteData> notes;
    for (int i = 0; i < count(); ++i) {
        const Note &note = m_data[i];
        auto it = m_timelineModels.find(note.timelineModel);
        if (it == m_timelineModels.end())
            continue;

        const QmlProfilerTimelineModel *model = it.value();
        QmlProfilerDataModel::QmlEventNoteData save = {
            model->typeId(note.timelineIndex), model->startTime(note.timelineIndex),
            model->duration(note.timelineIndex), note.text
        };
        notes.append(save);
    }
    m_modelManager->qmlModel()->setNoteData(notes);
    m_modified = false;
}
}
