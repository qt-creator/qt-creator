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

#include "timelinemodelaggregator.h"

#include "qmlprofilertimelinemodelproxy.h"
#include "qmlprofilerpainteventsmodelproxy.h"
#include "qmlprofilerplugin.h"

#include <QStringList>
#include <QVariant>

namespace QmlProfiler {
namespace Internal {


class TimelineModelAggregator::TimelineModelAggregatorPrivate {
public:
    TimelineModelAggregatorPrivate(TimelineModelAggregator *qq):q(qq) {}
    ~TimelineModelAggregatorPrivate() {}

    TimelineModelAggregator *q;

    int basicModelIndex;
    QList <AbstractTimelineModel *> modelList;
    QmlProfilerModelManager *modelManager;
};

TimelineModelAggregator::TimelineModelAggregator(QObject *parent)
    : QObject(parent), d(new TimelineModelAggregatorPrivate(this))
{
}

TimelineModelAggregator::~TimelineModelAggregator()
{
    delete d;
}

void TimelineModelAggregator::setModelManager(QmlProfilerModelManager *modelManager)
{
    d->modelManager = modelManager;
    connect(modelManager,SIGNAL(stateChanged()),this,SLOT(dataChanged()));
    connect(modelManager,SIGNAL(dataAvailable()),this,SIGNAL(dataAvailable()));

    // external models pushed on top
    foreach (AbstractTimelineModel *timelineModel, QmlProfilerPlugin::instance->getModels()) {
        timelineModel->setModelManager(modelManager);
        addModel(timelineModel);
    }

    PaintEventsModelProxy *paintEventsModelProxy = new PaintEventsModelProxy(this);
    paintEventsModelProxy->setModelManager(modelManager);
    addModel(paintEventsModelProxy);

    BasicTimelineModel *basicTimelineModel = new BasicTimelineModel(this);
    basicTimelineModel->setModelManager(modelManager);
    addModel(basicTimelineModel);
    // the basic model is the last one here
    d->basicModelIndex = d->modelList.count() - 1;

}

void TimelineModelAggregator::addModel(AbstractTimelineModel *m)
{
    d->modelList << m;
    connect(m,SIGNAL(emptyChanged()),this,SIGNAL(emptyChanged()));
    connect(m,SIGNAL(expandedChanged()),this,SIGNAL(expandedChanged()));
    connect(m,SIGNAL(stateChanged()),this,SIGNAL(stateChanged()));
}

// order?
int TimelineModelAggregator::categoryCount() const
{
    int categoryCount = 0;
    foreach (const AbstractTimelineModel *modelProxy, d->modelList)
        categoryCount += modelProxy->categoryCount();
    return categoryCount;
}

int TimelineModelAggregator::visibleCategories() const
{
    int categoryCount = 0;
    foreach (const AbstractTimelineModel *modelProxy, d->modelList) {
        for (int i = 0; i < modelProxy->categoryCount(); i++)
            if (modelProxy->categoryDepth(i) > 0)
                categoryCount ++;
    }
    return categoryCount;
}

QStringList TimelineModelAggregator::categoryTitles() const
{
    QStringList retString;
    foreach (const AbstractTimelineModel *modelProxy, d->modelList)
        retString += modelProxy->categoryTitles();
    return retString;
}

int TimelineModelAggregator::count(int modelIndex) const
{
    if (modelIndex == -1) {
        int totalCount = 0;
        foreach (const AbstractTimelineModel *modelProxy, d->modelList)
            totalCount += modelProxy->count();

        return totalCount;
    } else {
        return d->modelList[modelIndex]->count();
    }
}

bool TimelineModelAggregator::isEmpty() const
{
    foreach (const AbstractTimelineModel *modelProxy, d->modelList)
        if (!modelProxy->isEmpty())
            return false;
    return true;
}

bool TimelineModelAggregator::eventAccepted(const QmlProfilerDataModel::QmlEventData &/*event*/) const
{
    // accept all events
    return true;
}

int TimelineModelAggregator::basicModelIndex() const
{
    return d->basicModelIndex;
}

qint64 TimelineModelAggregator::lastTimeMark() const
{
    qint64 mark = -1;
    foreach (const AbstractTimelineModel *modelProxy, d->modelList) {
        if (!modelProxy->isEmpty()) {
            qint64 mk = modelProxy->lastTimeMark();
            if (mark > mk)
                mark = mk;
        }
    }
    return mark;
}

bool TimelineModelAggregator::expanded(int modelIndex, int category) const
{
    return d->modelList[modelIndex]->expanded(category);
}

void TimelineModelAggregator::setExpanded(int modelIndex, int category, bool expanded)
{
    d->modelList[modelIndex]->setExpanded(category, expanded);
}

int TimelineModelAggregator::categoryDepth(int modelIndex, int categoryIndex) const
{
    return d->modelList[modelIndex]->categoryDepth(categoryIndex);
}

int TimelineModelAggregator::categoryCount(int modelIndex) const
{
    return d->modelList[modelIndex]->categoryCount();
}

int TimelineModelAggregator::rowCount(int modelIndex) const
{
    return d->modelList[modelIndex]->rowCount();
}

const QString TimelineModelAggregator::categoryLabel(int modelIndex, int categoryIndex) const
{
    return d->modelList[modelIndex]->categoryLabel(categoryIndex);
}

int TimelineModelAggregator::modelIndexForCategory(int absoluteCategoryIndex) const
{
    int categoryIndex = absoluteCategoryIndex;
    for (int modelIndex = 0; modelIndex < d->modelList.count(); modelIndex++)
        if (categoryIndex < d->modelList[modelIndex]->categoryCount())
            return modelIndex;
        else
            categoryIndex -= d->modelList[modelIndex]->categoryCount();

    return modelCount()-1;
}

int TimelineModelAggregator::correctedCategoryIndexForModel(int modelIndex, int absoluteCategoryIndex) const
{
    int categoryIndex = absoluteCategoryIndex;
    for (int mi = 0; mi < modelIndex; mi++)
        categoryIndex -= d->modelList[mi]->categoryCount();
    return categoryIndex;
}

int TimelineModelAggregator::findFirstIndex(int modelIndex, qint64 startTime) const
{
    return d->modelList[modelIndex]->findFirstIndex(startTime);
}

int TimelineModelAggregator::findFirstIndexNoParents(int modelIndex, qint64 startTime) const
{
    return d->modelList[modelIndex]->findFirstIndexNoParents(startTime);
}

int TimelineModelAggregator::findLastIndex(int modelIndex, qint64 endTime) const
{
    return d->modelList[modelIndex]->findLastIndex(endTime);
}

int TimelineModelAggregator::getEventType(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->getEventType(index);
}

int TimelineModelAggregator::getEventCategoryInModel(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->getEventCategory(index);
}

int TimelineModelAggregator::getEventRow(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->getEventRow(index);
}

qint64 TimelineModelAggregator::getDuration(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->getDuration(index);
}

qint64 TimelineModelAggregator::getStartTime(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->getStartTime(index);
}

qint64 TimelineModelAggregator::getEndTime(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->getEndTime(index);
}

int TimelineModelAggregator::getEventId(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->getEventId(index);
}

int TimelineModelAggregator::getBindingLoopDest(int modelIndex,int index) const
{
    return d->modelList[modelIndex]->getBindingLoopDest(index);
}

QColor TimelineModelAggregator::getColor(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->getColor(index);
}

float TimelineModelAggregator::getHeight(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->getHeight(index);
}

const QVariantList TimelineModelAggregator::getLabelsForCategory(int modelIndex, int category) const
{
    return d->modelList[modelIndex]->getLabelsForCategory(category);
}

const QVariantList TimelineModelAggregator::getEventDetails(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->getEventDetails(index);
}

const QVariantMap TimelineModelAggregator::getEventLocation(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->getEventLocation(index);
}

int TimelineModelAggregator::getEventIdForHash(const QString &hash) const
{
    foreach (const AbstractTimelineModel *model, d->modelList) {
        int eventId = model->getEventIdForHash(hash);
        if (eventId != -1)
            return eventId;
    }
    return -1;
}

int TimelineModelAggregator::getEventIdForLocation(const QString &filename, int line, int column) const
{
    foreach (const AbstractTimelineModel *model, d->modelList) {
        int eventId = model->getEventIdForLocation(filename, line, column);
        if (eventId != -1)
            return eventId;
    }
    return -1;
}

void TimelineModelAggregator::dataChanged()
{
    // this is a slot connected for every modelproxy
    // nothing to do here, each model will take care of itself
}

int TimelineModelAggregator::modelCount() const
{
    return d->modelList.count();
}

qint64 TimelineModelAggregator::traceStartTime() const
{
    return d->modelManager->traceTime()->startTime();
}

qint64 TimelineModelAggregator::traceEndTime() const
{
    return d->modelManager->traceTime()->endTime();
}

qint64 TimelineModelAggregator::traceDuration() const
{
    return d->modelManager->traceTime()->duration();
}

int TimelineModelAggregator::getState() const
{
    return (int)d->modelManager->state();
}


} // namespace Internal
} // namespace QmlProfiler
