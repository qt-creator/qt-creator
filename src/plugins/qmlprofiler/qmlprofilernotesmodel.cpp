// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilernotesmodel.h"

#include <tracing/timelinemodel.h>
#include <utils/algorithm.h>

namespace QmlProfiler {

QmlProfilerNotesModel::QmlProfilerNotesModel(QObject *parent) : TimelineNotesModel(parent)
{
}

int QmlProfilerNotesModel::addQmlNote(int typeId, int collapsedRow, qint64 start, qint64 duration,
                                      const QString &text)
{
    qint64 difference = std::numeric_limits<qint64>::max();
    int foundTypeId = -1;
    int timelineModel = -1;
    int timelineIndex = -1;
    const QList<const Timeline::TimelineModel *> models = timelineModels();
    for (const Timeline::TimelineModel *model : models) {
        if (model->handlesTypeId(typeId)) {
            for (int i = model->firstIndex(start); i <= model->lastIndex(start + duration); ++i) {
                if (i < 0)
                    continue;
                if (collapsedRow != -1 && collapsedRow != model->collapsedRow(i))
                    continue;

                qint64 modelStart = model->startTime(i);
                qint64 modelDuration = model->duration(i);

                if (modelStart + modelDuration < start || start + duration < modelStart)
                    continue;

                // Accept different type IDs if row and time stamps match.
                // Some models base their type IDs on data from secondary events which may get
                // stripped by range restrictions.
                int modelTypeId = model->typeId(i);
                if (foundTypeId == typeId && modelTypeId != typeId)
                    continue;

                qint64 newDifference = qAbs(modelStart - start) + qAbs(modelDuration - duration);
                if (newDifference < difference) {
                    timelineModel = model->modelId();
                    timelineIndex = i;
                    difference = newDifference;
                    foundTypeId = modelTypeId;
                    if (difference == 0 && modelTypeId == typeId)
                        break;
                }
            }
            if (difference == 0 && foundTypeId == typeId)
                break;
        }
    }

    if (timelineModel == -1 || timelineIndex == -1)
        return -1;

    return TimelineNotesModel::add(timelineModel, timelineIndex, text);
}


void QmlProfilerNotesModel::restore()
{
    {
        QSignalBlocker blocker(this);
        for (int i = 0; i != m_notes.size(); ++i) {
            QmlNote &note = m_notes[i];
            note.setLoaded(addQmlNote(note.typeIndex(), note.collapsedRow(), note.startTime(),
                                      note.duration(), note.text()) != -1);
        }
        resetModified();
    }
    emit changed(-1, -1, -1);
}

void QmlProfilerNotesModel::stash()
{
    // Keep notes that are outside the given range, overwrite the ones inside the range.
    m_notes = Utils::filtered(m_notes, [](const QmlNote &note) {
        return !note.loaded();
    });

    for (int i = 0; i < count(); ++i) {
        const Timeline::TimelineModel *model = timelineModelByModelId(timelineModel(i));
        if (!model)
            continue;

        int index = timelineIndex(i);
        if (index < model->count()) {
            QmlNote save = {
                model->typeId(index),
                model->collapsedRow(index),
                model->startTime(index),
                model->duration(index),
                text(i)
            };
            m_notes.append(save);
        }
    }
    resetModified();
}

const QVector<QmlNote> &QmlProfilerNotesModel::notes() const
{
    return m_notes;
}

void QmlProfilerNotesModel::setNotes(const QVector<QmlNote> &notes)
{
    m_notes = notes;
}

void QmlProfilerNotesModel::addNote(const QmlNote &note)
{
    m_notes.append(note);
}

void QmlProfilerNotesModel::clear()
{
    TimelineNotesModel::clear();
    m_notes.clear();
}

} // namespace QmlProfiler
