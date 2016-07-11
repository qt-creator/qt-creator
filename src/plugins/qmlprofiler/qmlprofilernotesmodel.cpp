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

#include "qmlprofilernotesmodel.h"
#include "qmlprofilerdatamodel.h"

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
    foreach (const Timeline::TimelineModel *model, timelineModels()) {
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


void QmlProfilerNotesModel::loadData()
{
    blockSignals(true);
    for (int i = 0; i != m_notes.size(); ++i) {
        QmlNote &note = m_notes[i];
        note.setLoaded(addQmlNote(note.typeIndex(), note.collapsedRow(), note.startTime(),
                                  note.duration(), note.text()) != -1);
    }
    resetModified();
    blockSignals(false);
    emit changed(-1, -1, -1);
}

void QmlProfilerNotesModel::saveData()
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
        QmlNote save = {
            model->typeId(index),
            model->collapsedRow(index),
            model->startTime(index),
            model->duration(index),
            text(i)
        };
        m_notes.append(save);
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

void QmlProfilerNotesModel::clear()
{
    TimelineNotesModel::clear();
    m_notes.clear();
}

} // namespace QmlProfiler
