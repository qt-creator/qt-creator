// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinenotesmodel.h"
#include "timelinemodel.h"

namespace Timeline {

TimelineNotesModel::TimelineNotesModel(QObject *parent) : QObject(parent)
{
}

int TimelineNotesModel::count() const
{
    return m_data.count();
}

void TimelineNotesModel::addTimelineModel(const TimelineModel *timelineModel)
{
    connect(timelineModel, &QObject::destroyed, this, [this](QObject *obj) {
        removeTimelineModel(static_cast<TimelineModel *>(obj));
    });
    m_timelineModels.insert(timelineModel->modelId(), timelineModel);
}

const TimelineModel *TimelineNotesModel::timelineModelByModelId(int modelId) const
{
    auto it = m_timelineModels.find(modelId);
    return it == m_timelineModels.end() ? nullptr : it.value();
}

QList<const TimelineModel *> TimelineNotesModel::timelineModels() const
{
    return m_timelineModels.values();
}

int TimelineNotesModel::typeId(int index) const
{
    const Note &note = m_data[index];
    const TimelineModel *model = timelineModelByModelId(note.timelineModel);
    if (!model || note.timelineIndex >= model->count())
        return -1; // This can happen if one of the timeline models has been removed
    return model->typeId(note.timelineIndex);
}

QString TimelineNotesModel::text(int index) const
{
    return m_data[index].text;
}

int TimelineNotesModel::timelineModel(int index) const
{
    return m_data[index].timelineModel;
}

int TimelineNotesModel::timelineIndex(int index) const
{
    return m_data[index].timelineIndex;
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
    QVariantList ret;
    for (int noteId = 0; noteId < count(); ++noteId) {
        if (m_data[noteId].timelineModel == modelId)
            ret << noteId;
    }
    return ret;
}

int TimelineNotesModel::get(int modelId, int timelineIndex) const
{
    for (int noteId = 0; noteId < count(); ++noteId) {
        const Note &note = m_data[noteId];
        if (note.timelineModel == modelId && note.timelineIndex == timelineIndex)
            return noteId;
    }
    return -1;
}

int TimelineNotesModel::add(int modelId, int timelineIndex, const QString &text)
{
    const TimelineModel *model = m_timelineModels.value(modelId);
    int tid = model->typeId(timelineIndex);
    m_data << Note{text, modelId, timelineIndex};
    m_modified = true;
    emit changed(tid, modelId, timelineIndex);
    return m_data.count() - 1;
}

void TimelineNotesModel::update(int index, const QString &text)
{
    Note &note = m_data[index];
    if (text != note.text) {
        note.text = text;
        m_modified = true;
        emit changed(typeId(index), note.timelineModel, note.timelineIndex);
    }
}

void TimelineNotesModel::remove(int index)
{
    Note &note = m_data[index];
    int noteType = typeId(index);
    int modelId = note.timelineModel;
    int itemIndex = note.timelineIndex;
    m_data.removeAt(index);
    m_modified = true;
    emit changed(noteType, modelId, itemIndex);
}

bool TimelineNotesModel::isModified() const
{
    return m_modified;
}

void TimelineNotesModel::resetModified()
{
    m_modified = false;
}

void TimelineNotesModel::stash()
{
}

void TimelineNotesModel::restore()
{
}

void TimelineNotesModel::removeTimelineModel(const TimelineModel *timelineModel)
{
    for (auto i = m_timelineModels.begin(); i != m_timelineModels.end();) {
        if (i.value() == timelineModel)
            i = m_timelineModels.erase(i);
        else
            ++i;
    }
}

void TimelineNotesModel::setText(int noteId, const QString &text)
{
    if (text.size() > 0)
        update(noteId, text);
    else
        remove(noteId);
}

void TimelineNotesModel::setText(int modelId, int index, const QString &text)
{
    int noteId = get(modelId, index);
    if (noteId == -1) {
        if (text.size() > 0)
            add(modelId, index, text);
    } else {
        setText(noteId, text);
    }
}

void TimelineNotesModel::clear()
{
    m_data.clear();
    m_modified = false;
    emit changed(-1, -1, -1);
}

} // namespace Timeline

#include "moc_timelinenotesmodel.cpp"
