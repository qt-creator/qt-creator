// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinemodelaggregator.h"

#include "timelinemodel.h"
#include "timelinenotesmodel.h"

#include <QPointer>

namespace Timeline {

class TimelineModelAggregatorPrivate
{
public:
    QList <TimelineModel *> modelList;
    QPointer<TimelineNotesModel> notesModel;
    int currentModelId = 0;
};

TimelineModelAggregator::TimelineModelAggregator(QObject *parent)
    : QObject(parent), d(new TimelineModelAggregatorPrivate)
{
}

TimelineModelAggregator::~TimelineModelAggregator()
{
    delete d;
}

int TimelineModelAggregator::height() const
{
    return modelOffset(d->modelList.length());
}

int TimelineModelAggregator::generateModelId()
{
    return d->currentModelId++;
}

void TimelineModelAggregator::addModel(TimelineModel *m)
{
    d->modelList << m;
    connect(m, &TimelineModel::heightChanged, this, &TimelineModelAggregator::heightChanged);
    if (d->notesModel)
        d->notesModel->addTimelineModel(m);
    emit modelsChanged();
    if (m->height() != 0)
        emit heightChanged();
}

void TimelineModelAggregator::setModels(const QList<TimelineModel *> &models)
{
    if (d->modelList == models)
        return;

    int prevHeight = height();
    for (TimelineModel *m : std::as_const(d->modelList)) {
        disconnect(m, &TimelineModel::heightChanged, this, &TimelineModelAggregator::heightChanged);
        if (d->notesModel)
            d->notesModel->removeTimelineModel(m);
    }

    d->modelList = models;
    for (TimelineModel *m : models) {
        connect(m, &TimelineModel::heightChanged, this, &TimelineModelAggregator::heightChanged);
        if (d->notesModel)
            d->notesModel->addTimelineModel(m);
    }
    emit modelsChanged();
    if (height() != prevHeight)
        emit heightChanged();
}

const TimelineModel *TimelineModelAggregator::model(int modelIndex) const
{
    return d->modelList[modelIndex];
}

QList<TimelineModel *> TimelineModelAggregator::models() const
{
    return d->modelList;
}

TimelineNotesModel *TimelineModelAggregator::notes() const
{
    return d->notesModel;
}

void TimelineModelAggregator::setNotes(TimelineNotesModel *notes)
{
    if (d->notesModel == notes)
        return;

    if (d->notesModel) {
        disconnect(d->notesModel, &QObject::destroyed,
                   this, &TimelineModelAggregator::notesChanged);
    }

    d->notesModel = notes;

    if (d->notesModel)
        connect(d->notesModel, &QObject::destroyed, this, &TimelineModelAggregator::notesChanged);

    emit notesChanged();
}

void TimelineModelAggregator::clear()
{
    int prevHeight = height();
    d->modelList.clear();
    if (d->notesModel)
        d->notesModel->clear();
    emit modelsChanged();
    if (height() != prevHeight)
        emit heightChanged();
}

void TimelineModelAggregator::moveModel(int from, int to)
{
    if (from == to || from < 0 || from >= d->modelList.size()
            || to < 0 || to >= d->modelList.size())
        return;
    d->modelList.move(from, to);
    emit modelsChanged();
}

int TimelineModelAggregator::modelOffset(int modelIndex) const
{
    int ret = 0;
    for (int i = 0; i < modelIndex; ++i)
        ret += d->modelList[i]->height();
    return ret;
}

int TimelineModelAggregator::modelCount() const
{
    return d->modelList.count();
}

int TimelineModelAggregator::modelIndexById(int modelId) const
{
    for (int i = 0; i < d->modelList.count(); ++i) {
        if (d->modelList.at(i)->modelId() == modelId)
            return i;
    }
    return -1;
}

ModelItem TimelineModelAggregator::nextItem(int selectedModel, int selectedItem,
                                                   qint64 time) const
{
    if (selectedItem != -1)
        time = model(selectedModel)->startTime(selectedItem);

    QVarLengthArray<int> itemIndexes(modelCount());
    for (int i = 0; i < modelCount(); i++) {
        const TimelineModel *currentModel = model(i);
        if (currentModel->count() > 0) {
            if (selectedModel == i) {
                itemIndexes[i] = (selectedItem + 1) % currentModel->count();
            } else {
                if (currentModel->startTime(0) >= time)
                    itemIndexes[i] = 0;
                else
                    itemIndexes[i] = (currentModel->lastIndex(time) + 1) % currentModel->count();

                if (i < selectedModel && currentModel->startTime(itemIndexes[i]) == time)
                    itemIndexes[i] = (itemIndexes[i] + 1) % currentModel->count();
            }
        } else {
            itemIndexes[i] = -1;
        }
    }

    int candidateModelIndex = -1;
    qint64 candidateStartTime = std::numeric_limits<qint64>::max();
    for (int i = 0; i < modelCount(); i++) {
        if (itemIndexes[i] == -1)
            continue;
        qint64 newStartTime = model(i)->startTime(itemIndexes[i]);
        if (newStartTime < candidateStartTime &&
                (newStartTime > time || (newStartTime == time && i > selectedModel))) {
            candidateStartTime = newStartTime;
            candidateModelIndex = i;
        }
    }

    int itemIndex;
    if (candidateModelIndex != -1) {
        itemIndex = itemIndexes[candidateModelIndex];
    } else {
        itemIndex = -1;
        candidateStartTime = std::numeric_limits<qint64>::max();
        for (int i = 0; i < modelCount(); i++) {
            const TimelineModel *currentModel = model(i);
            if (currentModel->count() > 0 && currentModel->startTime(0) < candidateStartTime) {
                candidateModelIndex = i;
                itemIndex = 0;
                candidateStartTime = currentModel->startTime(0);
            }
        }
    }

    return {candidateModelIndex, itemIndex};
}

ModelItem TimelineModelAggregator::prevItem(int selectedModel, int selectedItem,
                                                   qint64 time) const
{
    if (selectedItem != -1)
        time = model(selectedModel)->startTime(selectedItem);

    QVarLengthArray<int> itemIndexes(modelCount());
    for (int i = 0; i < modelCount(); i++) {
        const TimelineModel *currentModel = model(i);
        if (selectedModel == i) {
            itemIndexes[i] = (selectedItem <= 0 ? currentModel->count() : selectedItem) - 1;
        } else {
            itemIndexes[i] = currentModel->lastIndex(time);
            if (itemIndexes[i] == -1)
                itemIndexes[i] = currentModel->count() - 1;
            else if (i < selectedModel && itemIndexes[i] + 1 < currentModel->count() &&
                     currentModel->startTime(itemIndexes[i] + 1) == time) {
                ++itemIndexes[i];
            }
        }
    }

    int candidateModelIndex = -1;
    qint64 candidateStartTime = std::numeric_limits<qint64>::min();
    for (int i = modelCount() - 1; i >= 0 ; --i) {
        const TimelineModel *currentModel = model(i);
        if (itemIndexes[i] == -1 || itemIndexes[i] >= currentModel->count())
            continue;
        qint64 newStartTime = currentModel->startTime(itemIndexes[i]);
        if (newStartTime > candidateStartTime &&
                (newStartTime < time || (newStartTime == time && i < selectedModel))) {
            candidateStartTime = newStartTime;
            candidateModelIndex = i;
        }
    }

    int itemIndex = -1;
    if (candidateModelIndex != -1) {
        itemIndex = itemIndexes[candidateModelIndex];
    } else {
        candidateStartTime = std::numeric_limits<qint64>::min();
        for (int i = 0; i < modelCount(); i++) {
            const TimelineModel *currentModel = model(i);
            if (currentModel->count() > 0 &&
                    currentModel->startTime(currentModel->count() - 1) > candidateStartTime) {
                candidateModelIndex = i;
                itemIndex = currentModel->count() - 1;
                candidateStartTime = currentModel->startTime(itemIndex);
            }
        }
    }

    return {candidateModelIndex, itemIndex};
}

} // namespace Timeline
