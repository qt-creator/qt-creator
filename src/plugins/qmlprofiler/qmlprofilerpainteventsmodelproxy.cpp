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

#include "qmlprofilerpainteventsmodelproxy.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilersimplemodel.h"
#include <QCoreApplication>

#include <QVector>
#include <QHash>
#include <QUrl>
#include <QString>
#include <QStack>

#include <QDebug>

namespace QmlProfiler {
namespace Internal {

struct CategorySpan {
    bool expanded;
    int expandedRows;
    int contractedRows;
};

class PaintEventsModelProxy::PaintEventsModelProxyPrivate
{
public:
    PaintEventsModelProxyPrivate(PaintEventsModelProxy *qq) : q(qq) {}
    ~PaintEventsModelProxyPrivate() {}

    QString displayTime(double time);
    void computeAnimationCountLimit();

    QVector <PaintEventsModelProxy::QmlPaintEventData> eventList;
    int minAnimationCount;
    int maxAnimationCount;
    bool expanded;

    PaintEventsModelProxy *q;
};

PaintEventsModelProxy::PaintEventsModelProxy(QObject *parent)
    : AbstractTimelineModel(parent), d(new PaintEventsModelProxyPrivate(this))
{
}

PaintEventsModelProxy::~PaintEventsModelProxy()
{
    delete d;
}

int PaintEventsModelProxy::categories() const
{
    return categoryCount();
}

QStringList PaintEventsModelProxy::categoryTitles() const
{
    QStringList retString;
    for (int i=0; i<categories(); i++)
        retString << categoryLabel(i);
    return retString;
}

QString PaintEventsModelProxy::name() const
{
    return QLatin1String("PaintEventsModelProxy");
}

const QVector<PaintEventsModelProxy::QmlPaintEventData> PaintEventsModelProxy::getData() const
{
    return d->eventList;
}

const QVector<PaintEventsModelProxy::QmlPaintEventData> PaintEventsModelProxy::getData(qint64 fromTime, qint64 toTime) const
{
    int fromIndex = findFirstIndex(fromTime);
    int toIndex = findLastIndex(toTime);
    if (fromIndex != -1 && toIndex > fromIndex)
        return d->eventList.mid(fromIndex, toIndex - fromIndex + 1);
    else
        return QVector<PaintEventsModelProxy::QmlPaintEventData>();
}

void PaintEventsModelProxy::clear()
{
    d->eventList.clear();
    d->minAnimationCount = 1;
    d->maxAnimationCount = 1;
    d->expanded = false;
    m_modelManager->modelProxyCountUpdated(m_modelId, 0, 1);
}

void PaintEventsModelProxy::dataChanged()
{
    if (m_modelManager->state() == QmlProfilerDataState::ProcessingData)
        loadData();

    if (m_modelManager->state() == QmlProfilerDataState::Empty)
        clear();

    emit stateChanged();
    emit dataAvailable();
    emit emptyChanged();
    emit expandedChanged();
}

bool compareStartTimes(const PaintEventsModelProxy::QmlPaintEventData &t1, const PaintEventsModelProxy::QmlPaintEventData &t2)
{
    return t1.startTime < t2.startTime;
}

bool PaintEventsModelProxy::eventAccepted(const QmlProfilerSimpleModel::QmlEventData &event) const
{
    return (event.eventType == QmlDebug::Painting && event.bindingType == QmlDebug::AnimationFrame);
}

void PaintEventsModelProxy::loadData()
{
    clear();
    QmlProfilerSimpleModel *simpleModel = m_modelManager->simpleModel();
    if (simpleModel->isEmpty())
        return;

    // collect events
    const QVector<QmlProfilerSimpleModel::QmlEventData> referenceList = simpleModel->getEvents();
    foreach (const QmlProfilerSimpleModel::QmlEventData &event, referenceList) {
        if (!eventAccepted(event))
            continue;

        qint64 estimatedDuration = 0;
        // initial estimation of the event duration: 1/framerate
        if (event.numericData1 > 0)
            estimatedDuration = 1e9/event.numericData1;

        // the profiler registers the animation events at the end of them
        qint64 realStartTime = event.startTime - estimatedDuration;

        // the duration of the events is estimated from the framerate
        // we need to correct it before appending a new event
        if (d->eventList.count() > 0) {
            QmlPaintEventData *lastEvent = &d->eventList[d->eventList.count()-1];
            if (lastEvent->startTime + lastEvent->duration >= realStartTime) {
                // 1 nanosecond less to prevent overlap
                lastEvent->duration = realStartTime - lastEvent->startTime - 1;
                lastEvent->framerate = 1e9/lastEvent->duration;
            }
        }

        QmlPaintEventData newEvent = {
            realStartTime,
            estimatedDuration,
            (int)event.numericData1,
            (int)event.numericData2
        };

        d->eventList.append(newEvent);

        m_modelManager->modelProxyCountUpdated(m_modelId, d->eventList.count(), referenceList.count());
    }

    d->computeAnimationCountLimit();

    qSort(d->eventList.begin(), d->eventList.end(), compareStartTimes);

    m_modelManager->modelProxyCountUpdated(m_modelId, 1, 1);

    emit countChanged();
}

/////////////////// QML interface

bool PaintEventsModelProxy::isEmpty() const
{
    return count() == 0;
}

int PaintEventsModelProxy::count() const
{
    return d->eventList.count();
}

qint64 PaintEventsModelProxy::lastTimeMark() const
{
    return d->eventList.last().startTime + d->eventList.last().duration;
}

bool PaintEventsModelProxy::expanded(int ) const
{
    return d->expanded;
}

void PaintEventsModelProxy::setExpanded(int category, bool expanded)
{
    Q_UNUSED(category);
    d->expanded = expanded;
    emit expandedChanged();
}

int PaintEventsModelProxy::categoryDepth(int categoryIndex) const
{
    Q_UNUSED(categoryIndex);
    if (isEmpty())
        return 0;
    else
        return 2;
}

int PaintEventsModelProxy::categoryCount() const
{
    return 1;
}

const QString PaintEventsModelProxy::categoryLabel(int categoryIndex) const
{
    Q_UNUSED(categoryIndex);
    return tr("Painting");
}


int PaintEventsModelProxy::findFirstIndex(qint64 startTime) const
{
    return findFirstIndexNoParents(startTime);
}

int PaintEventsModelProxy::findFirstIndexNoParents(qint64 startTime) const
{
    if (d->eventList.isEmpty())
        return -1;
    if (d->eventList.count() == 1 || d->eventList.first().startTime+d->eventList.first().duration >= startTime)
        return 0;
    else
        if (d->eventList.last().startTime+d->eventList.last().duration <= startTime)
            return -1;

    int fromIndex = 0;
    int toIndex = d->eventList.count()-1;
    while (toIndex - fromIndex > 1) {
        int midIndex = (fromIndex + toIndex)/2;
        if (d->eventList[midIndex].startTime + d->eventList[midIndex].duration < startTime)
            fromIndex = midIndex;
        else
            toIndex = midIndex;
    }
    return toIndex;
}

int PaintEventsModelProxy::findLastIndex(qint64 endTime) const
{
    if (d->eventList.isEmpty())
        return -1;
    if (d->eventList.first().startTime >= endTime)
        return -1;
    if (d->eventList.count() == 1)
        return 0;
    if (d->eventList.last().startTime <= endTime)
        return d->eventList.count()-1;

    int fromIndex = 0;
    int toIndex = d->eventList.count()-1;
    while (toIndex - fromIndex > 1) {
        int midIndex = (fromIndex + toIndex)/2;
        if (d->eventList[midIndex].startTime < endTime)
            fromIndex = midIndex;
        else
            toIndex = midIndex;
    }

    return fromIndex;
}

int PaintEventsModelProxy::getEventType(int index) const
{
    Q_UNUSED(index);
    return (int)QmlDebug::Painting;
}

int PaintEventsModelProxy::getEventCategory(int index) const
{
    Q_UNUSED(index);
    // there is only one category, all events belong to it
    return 0;
}

int PaintEventsModelProxy::getEventRow(int index) const
{
    Q_UNUSED(index);
    return 1;
}

qint64 PaintEventsModelProxy::getDuration(int index) const
{
    return d->eventList[index].duration;
}

qint64 PaintEventsModelProxy::getStartTime(int index) const
{
    return d->eventList[index].startTime;
}

qint64 PaintEventsModelProxy::getEndTime(int index) const
{
    return d->eventList[index].startTime + d->eventList[index].duration;
}

int PaintEventsModelProxy::getEventId(int index) const
{
    // there is only one event Id for all painting events
    Q_UNUSED(index);
    return 0;
}

QColor PaintEventsModelProxy::getColor(int index) const
{
    double fpsFraction = d->eventList[index].framerate / 60.0;
    if (fpsFraction > 1.0)
        fpsFraction = 1.0;
    if (fpsFraction < 0.0)
        fpsFraction = 0.0;
    return QColor::fromHsl((fpsFraction*96)+10, 76, 166);
}

float PaintEventsModelProxy::getHeight(int index) const
{
    float scale = d->maxAnimationCount - d->minAnimationCount;
    float fraction = 1.0f;
    if (scale > 1)
        fraction = (float)(d->eventList[index].animationcount -
                            d->minAnimationCount) / scale;

    return fraction * 0.85f + 0.15f;
}

const QVariantList PaintEventsModelProxy::getLabelsForCategory(int category) const
{
    Q_UNUSED(category);
    QVariantList result;

    if (!isEmpty()) {
        QVariantMap element;
        element.insert(QLatin1String("displayName"), QVariant(QLatin1String("Animations")));
        element.insert(QLatin1String("description"), QVariant(QLatin1String("Animations")));
        element.insert(QLatin1String("id"), QVariant(0));
        result << element;
    }

    return result;
}

QString PaintEventsModelProxy::PaintEventsModelProxyPrivate::displayTime(double time)
{
    if (time < 1e6)
        return QString::number(time/1e3,'f',3) + trUtf8(" \xc2\xb5s");
    if (time < 1e9)
        return QString::number(time/1e6,'f',3) + tr(" ms");

    return QString::number(time/1e9,'f',3) + tr(" s");
}

void PaintEventsModelProxy::PaintEventsModelProxyPrivate::computeAnimationCountLimit()
{
    minAnimationCount = 1;
    maxAnimationCount = 1;
    if (eventList.isEmpty())
        return;

    for (int i=0; i < eventList.count(); i++) {
        if (eventList[i].animationcount < minAnimationCount)
            minAnimationCount = eventList[i].animationcount;
        if (eventList[i].animationcount > maxAnimationCount)
            maxAnimationCount = eventList[i].animationcount;
    }
}

const QVariantList PaintEventsModelProxy::getEventDetails(int index) const
{
    QVariantList result;
//    int eventId = getEventId(index);

    static const char trContext[] = "RangeDetails";
    {
        QVariantMap valuePair;
        valuePair.insert(QLatin1String("title"), QVariant(categoryLabel(0)));
        result << valuePair;
    }

    // duration
    {
        QVariantMap valuePair;
        valuePair.insert(QCoreApplication::translate(trContext, "Duration:"), QVariant(d->displayTime(d->eventList[index].duration)));
        result << valuePair;
    }

    // duration
    {
        QVariantMap valuePair;
        valuePair.insert(QCoreApplication::translate(trContext, "Framerate:"), QVariant(QString::fromLatin1("%1 FPS").arg(d->eventList[index].framerate)));
        result << valuePair;
    }

    // duration
    {
        QVariantMap valuePair;
        valuePair.insert(QCoreApplication::translate(trContext, "Animations:"), QVariant(QString::fromLatin1("%1").arg(d->eventList[index].animationcount)));
        result << valuePair;
    }

    return result;
}

const QVariantMap PaintEventsModelProxy::getEventLocation(int /*index*/) const
{
    QVariantMap map;
    return map;
}

int PaintEventsModelProxy::getEventIdForHash(const QString &/*eventHash*/) const
{
    // paint events do not have an eventHash
    return -1;
}

int PaintEventsModelProxy::getEventIdForLocation(const QString &/*filename*/, int /*line*/, int /*column*/) const
{
    // paint events do not have a defined location
    return -1;
}

}
}

