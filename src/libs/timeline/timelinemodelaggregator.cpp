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

#include "timelinemodelaggregator.h"

#include "timelinemodel.h"
#include "timelinenotesmodel.h"

#include <QStringList>
#include <QVariant>

namespace Timeline {

class TimelineModelAggregator::TimelineModelAggregatorPrivate {
public:
    QList <TimelineModel *> modelList;
    TimelineNotesModel *notesModel;
};

TimelineModelAggregator::TimelineModelAggregator(TimelineNotesModel *notes, QObject *parent)
    : QObject(parent), d_ptr(new TimelineModelAggregatorPrivate)
{
    Q_D(TimelineModelAggregator);
    d->notesModel = notes;
}

TimelineModelAggregator::~TimelineModelAggregator()
{
    Q_D(TimelineModelAggregator);
    delete d;
}

int TimelineModelAggregator::height() const
{
    Q_D(const TimelineModelAggregator);
    return modelOffset(d->modelList.length());
}

void TimelineModelAggregator::addModel(TimelineModel *m)
{
    Q_D(TimelineModelAggregator);
    d->modelList << m;
    connect(m,SIGNAL(heightChanged()),this,SIGNAL(heightChanged()));
    if (d->notesModel)
        d->notesModel->addTimelineModel(m);
    emit modelsChanged();
    if (m->height() != 0)
        emit heightChanged();
}

const TimelineModel *TimelineModelAggregator::model(int modelIndex) const
{
    Q_D(const TimelineModelAggregator);
    return d->modelList[modelIndex];
}

QVariantList TimelineModelAggregator::models() const
{
    Q_D(const TimelineModelAggregator);
    QVariantList ret;
    foreach (TimelineModel *model, d->modelList)
        ret << QVariant::fromValue(model);
    return ret;
}

TimelineNotesModel *TimelineModelAggregator::notes() const
{
    Q_D(const TimelineModelAggregator);
    return d->notesModel;
}

void TimelineModelAggregator::clear()
{
    Q_D(TimelineModelAggregator);
    int prevHeight = height();
    d->modelList.clear();
    if (d->notesModel)
        d->notesModel->clear();
    emit modelsChanged();
    if (height() != prevHeight)
        emit heightChanged();
}

int TimelineModelAggregator::modelOffset(int modelIndex) const
{
    Q_D(const TimelineModelAggregator);
    int ret = 0;
    for (int i = 0; i < modelIndex; ++i)
        ret += d->modelList[i]->height();
    return ret;
}

int TimelineModelAggregator::modelCount() const
{
    Q_D(const TimelineModelAggregator);
    return d->modelList.count();
}

int TimelineModelAggregator::modelIndexById(int modelId) const
{
    Q_D(const TimelineModelAggregator);
    for (int i = 0; i < d->modelList.count(); ++i) {
        if (d->modelList.at(i)->modelId() == modelId)
            return i;
    }
    return -1;
}

QVariantMap TimelineModelAggregator::nextItem(int selectedModel, int selectedItem,
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

    QVariantMap ret;
    ret.insert(QLatin1String("model"), candidateModelIndex);
    ret.insert(QLatin1String("item"), itemIndex);
    return ret;
}

QVariantMap TimelineModelAggregator::prevItem(int selectedModel, int selectedItem,
                                              qint64 time) const
{
    if (selectedItem != -1)
        time = model(selectedModel)->startTime(selectedItem);

    QVarLengthArray<int> itemIndexes(modelCount());
    for (int i = 0; i < modelCount(); i++) {
        const TimelineModel *currentModel = model(i);
        if (selectedModel == i) {
            itemIndexes[i] = (selectedItem == 0 ? currentModel->count() : selectedItem) - 1;
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

    QVariantMap ret;
    ret.insert(QLatin1String("model"), candidateModelIndex);
    ret.insert(QLatin1String("item"), itemIndex);
    return ret;
}

} // namespace Timeline
