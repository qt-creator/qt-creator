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

namespace QmlProfiler {

QmlProfilerNotesModel::QmlProfilerNotesModel(QObject *parent) : TimelineNotesModel(parent),
    m_modelManager(0)
{
}

void QmlProfilerNotesModel::setModelManager(QmlProfilerModelManager *modelManager)
{
    m_modelManager = modelManager;
}

int QmlProfilerNotesModel::add(int typeId, qint64 start, qint64 duration, const QString &text)
{
    int timelineModel = -1;
    int timelineIndex = -1;
    foreach (const Timeline::TimelineModel *model, timelineModels()) {
        if (model->handlesTypeId(typeId)) {
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

    return TimelineNotesModel::add(timelineModel, timelineIndex, text);
}


void QmlProfilerNotesModel::loadData()
{
    blockSignals(true);
    clear();
    const QVector<QmlProfilerDataModel::QmlEventNoteData> &notes =
            m_modelManager->qmlModel()->getEventNotes();
    for (int i = 0; i != notes.size(); ++i) {
        const QmlProfilerDataModel::QmlEventNoteData &note = notes[i];
        add(note.typeIndex, note.startTime, note.duration, note.text);
    }
    resetModified();
    blockSignals(false);
    emit changed(-1, -1, -1);
}

void QmlProfilerNotesModel::saveData()
{
    QVector<QmlProfilerDataModel::QmlEventNoteData> notes;
    for (int i = 0; i < count(); ++i) {
        const Timeline::TimelineModel *model = timelineModelByModelId(timelineModel(i));
        if (!model)
            continue;

        int index = timelineIndex(i);
        QmlProfilerDataModel::QmlEventNoteData save = {
            model->typeId(index),
            model->startTime(index),
            model->duration(index),
            text(i)
        };
        notes.append(save);
    }
    m_modelManager->qmlModel()->setNoteData(notes);
    resetModified();
}
}
