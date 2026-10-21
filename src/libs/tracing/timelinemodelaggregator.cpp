// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinemodelaggregator.h"

#include "timelinemodel.h"
#include "timelinenotesmodel.h"

#include <utils/qtcsettings.h>

#include <QPointer>
#include <QSet>
#include <QVarLengthArray>

namespace Timeline {

// Models the aggregator remembers the place or the hidden state of, beyond
// which it forgets the ones that are not in the current trace.
const int kMaxRememberedModels = 256;

class TimelineModelAggregatorPrivate
{
public:
    QList<TimelineModel *> inRememberedOrder(const QList<TimelineModel *> &models) const;
    void rememberOrder();
    bool hideRemembered();
    void rememberHidden(const TimelineModel *model, bool hidden);
    QSet<QString> presentIds() const;
    void saveLayout() const;

    QList <TimelineModel *> modelList;
    QPointer<TimelineNotesModel> notesModel;
    QSet<int> hiddenModelIds;
    QStringList rememberedOrder;
    QSet<QString> rememberedHidden;
    Utils::QtcSettings *settings = nullptr;
    Utils::Key settingsKey;
    int currentModelId = 0;
    TimelineModelAggregator::PausedRanges pausedRanges;
};

/*
    Puts the models the user has given a place to into the places that these
    same models hold in models, in the order the user gave them, and leaves
    every other model where its owner put it.
*/
QList<TimelineModel *> TimelineModelAggregatorPrivate::inRememberedOrder(
    const QList<TimelineModel *> &models) const
{
    QHash<QString, int> rank;
    for (int i = 0; i < rememberedOrder.size(); ++i)
        rank.insert(rememberedOrder.at(i), i);

    QList<int> places;
    QList<TimelineModel *> placed;
    for (int i = 0; i < models.size(); ++i) {
        if (rank.contains(models.at(i)->persistentId())) {
            places.append(i);
            placed.append(models.at(i));
        }
    }
    std::stable_sort(placed.begin(), placed.end(),
                     [&rank](const TimelineModel *a, const TimelineModel *b) {
                         return rank.value(a->persistentId()) < rank.value(b->persistentId());
                     });

    QList<TimelineModel *> result = models;
    for (int i = 0; i < places.size(); ++i)
        result[places.at(i)] = placed.at(i);
    return result;
}

QSet<QString> TimelineModelAggregatorPrivate::presentIds() const
{
    QSet<QString> ids;
    for (const TimelineModel *model : modelList) {
        if (!model->persistentId().isEmpty())
            ids.insert(model->persistentId());
    }
    return ids;
}

/*
    Takes the order of the models on show into rememberedOrder. A model that is
    not on show keeps its place relative to the others there, so that a model
    which a trace lacks, or which its owner left out for now, comes back where
    it was. A model shown for the first time goes right after the one above it.
*/
void TimelineModelAggregatorPrivate::rememberOrder()
{
    QStringList shown;
    for (const TimelineModel *model : std::as_const(modelList)) {
        if (!model->persistentId().isEmpty())
            shown.append(model->persistentId());
    }
    const QSet<QString> shownSet(shown.cbegin(), shown.cend());
    QSet<QString> known(rememberedOrder.cbegin(), rememberedOrder.cend());

    QStringList order = rememberedOrder;
    auto nextShown = shown.cbegin();
    for (QString &id : order) {
        if (!shownSet.contains(id))
            continue;
        while (!known.contains(*nextShown))
            ++nextShown;
        id = *nextShown++;
    }

    int previous = -1;
    for (const QString &id : std::as_const(shown)) {
        if (known.contains(id)) {
            previous = order.indexOf(id);
        } else {
            order.insert(++previous, id);
            known.insert(id);
        }
    }

    if (order.size() > kMaxRememberedModels)
        order.removeIf([&shownSet](const QString &id) { return !shownSet.contains(id); });
    rememberedOrder = order;
}

bool TimelineModelAggregatorPrivate::hideRemembered()
{
    bool changed = false;
    for (const TimelineModel *model : std::as_const(modelList)) {
        if (model->persistentId().isEmpty())
            continue;
        const bool hidden = rememberedHidden.contains(model->persistentId());
        if (hidden == hiddenModelIds.contains(model->modelId()))
            continue;
        if (hidden)
            hiddenModelIds.insert(model->modelId());
        else
            hiddenModelIds.remove(model->modelId());
        changed = true;
    }
    return changed;
}

void TimelineModelAggregatorPrivate::rememberHidden(const TimelineModel *model, bool hidden)
{
    if (model->persistentId().isEmpty())
        return;
    if (hidden)
        rememberedHidden.insert(model->persistentId());
    else
        rememberedHidden.remove(model->persistentId());
    if (rememberedHidden.size() > kMaxRememberedModels)
        rememberedHidden.intersect(presentIds());
}

void TimelineModelAggregatorPrivate::saveLayout() const
{
    if (!settings)
        return;
    QStringList hidden(rememberedHidden.cbegin(), rememberedHidden.cend());
    hidden.sort();
    settings->setValueWithDefault(settingsKey + "/Order", rememberedOrder);
    settings->setValueWithDefault(settingsKey + "/Hidden", hidden);
}

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
    d->modelList.append(m);
    d->modelList = d->inRememberedOrder(d->modelList);
    connect(m, &TimelineModel::heightChanged, this, &TimelineModelAggregator::heightChanged);
    if (d->notesModel)
        d->notesModel->addTimelineModel(m);
    const bool hiddenChanged = d->hideRemembered();
    emit modelsChanged();
    if (hiddenChanged)
        emit hiddenModelsChanged();
    if (m->height() != 0)
        emit heightChanged();
}

/*!
    Shows \a models in the timeline, in the order they come in, apart from the
    models the user has moved: these take the places the user gave them among
    each other, keyed by TimelineModel::persistentId().
*/
void TimelineModelAggregator::setModels(const QList<TimelineModel *> &models)
{
    const QList<TimelineModel *> ordered = d->inRememberedOrder(models);
    if (d->modelList == ordered)
        return;

    int prevHeight = height();
    for (TimelineModel *m : std::as_const(d->modelList)) {
        disconnect(m, &TimelineModel::heightChanged, this, &TimelineModelAggregator::heightChanged);
        if (d->notesModel)
            d->notesModel->removeTimelineModel(m);
    }

    d->modelList = ordered;
    for (TimelineModel *m : ordered) {
        connect(m, &TimelineModel::heightChanged, this, &TimelineModelAggregator::heightChanged);
        if (d->notesModel)
            d->notesModel->addTimelineModel(m);
    }
    const bool hiddenChanged = d->hideRemembered();
    emit modelsChanged();
    if (hiddenChanged)
        emit hiddenModelsChanged();
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

const TimelineModelAggregator::PausedRanges &TimelineModelAggregator::pausedRanges() const
{
    return d->pausedRanges;
}

void TimelineModelAggregator::setPausedRanges(const PausedRanges &ranges)
{
    if (d->pausedRanges == ranges)
        return;
    d->pausedRanges = ranges;
    emit pausedRangesChanged();
}

void TimelineModelAggregator::clear()
{
    int prevHeight = height();
    d->modelList.clear();
    if (d->notesModel)
        d->notesModel->clear();
    setPausedRanges({});
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
    d->rememberOrder();
    d->saveLayout();
    emit modelsChanged();
}

/*!
    Returns whether the user has taken the model with \a modelId out of the
    timeline. Unlike TimelineModel::hidden(), which the owner of a model controls,
    this is a choice of the person looking at the timeline. A hidden model keeps
    its place in models(), so showing it again puts it back where it was.

    A model with a TimelineModel::persistentId() stays hidden when it is shown
    again for another trace.
*/
bool TimelineModelAggregator::isModelHidden(int modelId) const
{
    return d->hiddenModelIds.contains(modelId);
}

void TimelineModelAggregator::setModelHidden(int modelId, bool hidden)
{
    if (hidden == d->hiddenModelIds.contains(modelId))
        return;
    if (hidden)
        d->hiddenModelIds.insert(modelId);
    else
        d->hiddenModelIds.remove(modelId);
    const int modelIndex = modelIndexById(modelId);
    if (modelIndex != -1) {
        d->rememberHidden(d->modelList.at(modelIndex), hidden);
        d->saveLayout();
    }
    emit hiddenModelsChanged();
}

void TimelineModelAggregator::showOnlyModel(int modelId)
{
    QSet<int> hidden;
    for (const TimelineModel *model : std::as_const(d->modelList)) {
        if (model->modelId() != modelId)
            hidden.insert(model->modelId());
    }
    if (hidden == d->hiddenModelIds)
        return;
    d->hiddenModelIds = hidden;
    for (const TimelineModel *model : std::as_const(d->modelList))
        d->rememberHidden(model, model->modelId() != modelId);
    d->saveLayout();
    emit hiddenModelsChanged();
}

void TimelineModelAggregator::showAllModels()
{
    if (d->hiddenModelIds.isEmpty())
        return;
    d->hiddenModelIds.clear();
    for (const TimelineModel *model : std::as_const(d->modelList))
        d->rememberHidden(model, false);
    d->saveLayout();
    emit hiddenModelsChanged();
}

/*!
    Keeps the order and the hidden state of the models in \a settings under
    \a key, and takes them from there now. Only models with a
    TimelineModel::persistentId() are kept. The settings must outlive the
    aggregator.
*/
void TimelineModelAggregator::persistLayout(Utils::QtcSettings *settings, const Utils::Key &key)
{
    d->settings = settings;
    d->settingsKey = key;
    d->rememberedOrder = settings->value(key + "/Order").toStringList();
    const QStringList hidden = settings->value(key + "/Hidden").toStringList();
    d->rememberedHidden = QSet<QString>(hidden.cbegin(), hidden.cend());

    const QList<TimelineModel *> models = d->inRememberedOrder(d->modelList);
    const bool orderChanged = models != d->modelList;
    d->modelList = models;
    const bool hiddenChanged = d->hideRemembered();
    if (orderChanged)
        emit modelsChanged();
    if (hiddenChanged)
        emit hiddenModelsChanged();
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
