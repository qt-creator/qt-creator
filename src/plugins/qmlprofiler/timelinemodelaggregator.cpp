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

    for (int i = 0; i < QmlDebug::MaximumRangeType; ++i) {
        RangeTimelineModel *rangeModel = new RangeTimelineModel((QmlDebug::RangeType)i, this);
        rangeModel->setModelManager(modelManager);
        addModel(rangeModel);
    }

    // Connect this last so that it's executed after the models have updated their data.
    connect(modelManager->qmlModel(),SIGNAL(changed()),this,SIGNAL(stateChanged()));
}

void TimelineModelAggregator::addModel(AbstractTimelineModel *m)
{
    d->modelList << m;
    connect(m,SIGNAL(expandedChanged()),this,SIGNAL(expandedChanged()));
    connect(m,SIGNAL(rowHeightChanged()),this,SIGNAL(rowHeightChanged()));
}

QStringList TimelineModelAggregator::categoryTitles() const
{
    QStringList retString;
    foreach (const AbstractTimelineModel *modelProxy, d->modelList)
        retString << modelProxy->title();
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

int TimelineModelAggregator::height(int modelIndex) const
{
    return d->modelList[modelIndex]->height();
}

int TimelineModelAggregator::rowHeight(int modelIndex, int row) const
{
    return d->modelList[modelIndex]->rowHeight(row);
}

int TimelineModelAggregator::rowOffset(int modelIndex, int row) const
{
    return d->modelList[modelIndex]->rowOffset(row);
}

void TimelineModelAggregator::setRowHeight(int modelIndex, int row, int height)
{
    d->modelList[modelIndex]->setRowHeight(row, height);
}

bool TimelineModelAggregator::expanded(int modelIndex) const
{
    return d->modelList[modelIndex]->expanded();
}

void TimelineModelAggregator::setExpanded(int modelIndex, bool expanded)
{
    d->modelList[modelIndex]->setExpanded(expanded);
}

int TimelineModelAggregator::rowCount(int modelIndex) const
{
    return d->modelList[modelIndex]->rowCount();
}

const QString TimelineModelAggregator::title(int modelIndex) const
{
    return d->modelList[modelIndex]->title();
}

int TimelineModelAggregator::rowMinValue(int modelIndex, int row) const
{
    return d->modelList[modelIndex]->rowMinValue(row);
}

int TimelineModelAggregator::rowMaxValue(int modelIndex, int row) const
{
    return d->modelList[modelIndex]->rowMaxValue(row);
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

const QVariantList TimelineModelAggregator::getLabels(int modelIndex) const
{
    return d->modelList[modelIndex]->getLabels();
}

const QVariantList TimelineModelAggregator::getEventDetails(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->getEventDetails(index);
}

const QVariantMap TimelineModelAggregator::getEventLocation(int modelIndex, int index) const
{
    return d->modelList[modelIndex]->getEventLocation(index);
}

int TimelineModelAggregator::getEventIdForTypeIndex(int modelIndex, int typeIndex) const
{
    return d->modelList[modelIndex]->getEventIdForTypeIndex(typeIndex);
}

int TimelineModelAggregator::getEventIdForLocation(int modelIndex, const QString &filename,
                                                   int line, int column) const
{
    return d->modelList[modelIndex]->getEventIdForLocation(filename, line, column);
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
