/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmlprofilereventlist.h"

#include <QtCore/QUrl>
#include <QtCore/QHash>
#include <QtCore/QtAlgorithms>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <QtCore/QFile>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>

#include <QtCore/QTimer>

#include <QDebug>

namespace QmlJsDebugClient {

namespace Constants {
const char *const TYPE_PAINTING_STR = "Painting";
const char *const TYPE_COMPILING_STR = "Compiling";
const char *const TYPE_CREATING_STR = "Creating";
const char *const TYPE_BINDING_STR = "Binding";
const char *const TYPE_HANDLINGSIGNAL_STR = "HandlingSignal";
const char *const PROFILER_FILE_VERSION = "1.01";
}

#define MIN_LEVEL 1

QmlEventData::QmlEventData()
{
    line = -1;
    eventType = MaximumQmlEventType;
    eventId = -1;
    duration = 0;
    calls = 0;
    minTime = 0;
    maxTime = 0;
    timePerCall = 0;
    percentOfTime = 0;
    medianTime = 0;
}

QmlEventData::~QmlEventData()
{
    qDeleteAll(parentHash.values());
    parentHash.clear();
    qDeleteAll(childrenHash.values());
    childrenHash.clear();
}

QmlEventData &QmlEventData::operator=(const QmlEventData &ref)
{
    if (this == &ref)
        return *this;

    displayname = ref.displayname;
    filename = ref.filename;
    eventHashStr = ref.eventHashStr;
    details = ref.details;
    line = ref.line;
    eventType = ref.eventType;
    duration = ref.duration;
    calls = ref.calls;
    minTime = ref.minTime;
    maxTime = ref.maxTime;
    timePerCall = ref.timePerCall;
    percentOfTime = ref.percentOfTime;
    medianTime = ref.medianTime;
    eventId = ref.eventId;

    qDeleteAll(parentHash.values());
    parentHash.clear();
    foreach (const QString &key, ref.parentHash.keys()) {
        parentHash.insert(key, new QmlEventSub(ref.parentHash.value(key)));
    }

    qDeleteAll(childrenHash.values());
    childrenHash.clear();
    foreach (const QString &key, ref.childrenHash.keys()) {
        childrenHash.insert(key, new QmlEventSub(ref.childrenHash.value(key)));
    }

    return *this;
}

QV8EventData::QV8EventData()
{
    line = -1;
    eventId = -1;
    totalTime = 0;
    selfTime = 0;
    totalPercent = 0;
    selfPercent = 0;
}

QV8EventData::~QV8EventData()
{
    qDeleteAll(parentHash.values());
    parentHash.clear();
    qDeleteAll(childrenHash.values());
    childrenHash.clear();
}

QV8EventData &QV8EventData::operator=(const QV8EventData &ref)
{
    if (this == &ref)
        return *this;

    displayName = ref.displayName;
    filename = ref.filename;
    functionName = ref.functionName;
    line = ref.line;
    totalTime = ref.totalTime;
    totalPercent = ref.totalPercent;
    selfTime = ref.selfTime;
    selfPercent = ref.selfPercent;
    eventId = ref.eventId;

    qDeleteAll(parentHash.values());
    parentHash.clear();
    foreach (const QString &key, ref.parentHash.keys()) {
        parentHash.insert(key, new QV8EventSub(ref.parentHash.value(key)));
    }

    qDeleteAll(childrenHash.values());
    childrenHash.clear();
    foreach (const QString &key, ref.childrenHash.keys()) {
        childrenHash.insert(key, new QV8EventSub(ref.childrenHash.value(key)));
    }
    return *this;
}

// endtimedata
struct QmlEventEndTimeData {
    qint64 endTime;
    int startTimeIndex;
    QmlEventData *description;
};

// starttimedata
struct QmlEventStartTimeData {
    qint64 startTime;
    qint64 length;
    qint64 level;
    int endTimeIndex;
    qint64 nestingLevel;
    qint64 nestingDepth;
    QmlEventData *description;

};

struct QmlEventTypeCount {
    QList <int> eventIds;
    int nestingCount;
};

// used by quicksort
bool compareEndTimes(const QmlEventEndTimeData &t1, const QmlEventEndTimeData &t2)
{
    return t1.endTime < t2.endTime;
}

bool compareStartTimes(const QmlEventStartTimeData &t1, const QmlEventStartTimeData &t2)
{
    return t1.startTime < t2.startTime;
}

bool compareStartIndexes(const QmlEventEndTimeData &t1, const QmlEventEndTimeData &t2)
{
    return t1.startTimeIndex < t2.startTimeIndex;
}

QString qmlEventType(QmlEventType typeEnum)
{
    switch (typeEnum) {
    case Painting:
        return QLatin1String(Constants::TYPE_PAINTING_STR);
        break;
    case Compiling:
        return QLatin1String(Constants::TYPE_COMPILING_STR);
        break;
    case Creating:
        return QLatin1String(Constants::TYPE_CREATING_STR);
        break;
    case Binding:
        return QLatin1String(Constants::TYPE_BINDING_STR);
        break;
    case HandlingSignal:
        return QLatin1String(Constants::TYPE_HANDLINGSIGNAL_STR);
        break;
    default:
        return QString::number((int)typeEnum);
    }
}

QmlEventType qmlEventType(const QString &typeString)
{
    if (typeString == QLatin1String(Constants::TYPE_PAINTING_STR)) {
        return Painting;
    } else if (typeString == QLatin1String(Constants::TYPE_COMPILING_STR)) {
        return Compiling;
    } else if (typeString == QLatin1String(Constants::TYPE_CREATING_STR)) {
        return Creating;
    } else if (typeString == QLatin1String(Constants::TYPE_BINDING_STR)) {
        return Binding;
    } else if (typeString == QLatin1String(Constants::TYPE_HANDLINGSIGNAL_STR)) {
        return HandlingSignal;
    } else {
        bool isNumber = false;
        int type = typeString.toUInt(&isNumber);
        if (isNumber) {
            return (QmlEventType)type;
        } else {
            return MaximumQmlEventType;
        }
    }
}

class QmlProfilerEventList::QmlProfilerEventListPrivate
{
public:
    QmlProfilerEventListPrivate(QmlProfilerEventList *qq) : q(qq) {}

    QmlProfilerEventList *q;

    // convenience functions
    void clearQmlRootEvent();
    void clearV8RootEvent();

    // Stored data
    QmlEventHash m_eventDescriptions;
    QList<QmlEventEndTimeData> m_endTimeSortedList;
    QList<QmlEventStartTimeData> m_startTimeSortedList;

    void collectV8Statistics();
    QV8EventDescriptions m_v8EventList;
    QHash<int, QV8EventData *> m_v8parents;

    QmlEventData m_qmlRootEvent;
    QV8EventData m_v8RootEvent;
    QString m_rootEventName;
    QString m_rootEventDesc;

    QHash<int, QmlEventTypeCount *> m_typeCounts;

    qint64 m_traceEndTime;
    qint64 m_traceStartTime;
    qint64 m_qmlMeasuredTime;
    qint64 m_v8MeasuredTime;

    // file to load
    QString m_filename;
};


////////////////////////////////////////////////////////////////////////////////////


QmlProfilerEventList::QmlProfilerEventList(QObject *parent) :
    QObject(parent), d(new QmlProfilerEventListPrivate(this))
{
    setObjectName("QmlProfilerEventStatistics");

    d->m_traceEndTime = 0;
    d->m_traceStartTime = -1;
    d->m_qmlMeasuredTime = 0;
    d->m_v8MeasuredTime = 0;
    d->m_rootEventName = tr("<program>");
    d->m_rootEventDesc = tr("Main Program");
    d->clearQmlRootEvent();
    d->clearV8RootEvent();

}

QmlProfilerEventList::~QmlProfilerEventList()
{
    clear();
}

void QmlProfilerEventList::clear()
{
    qDeleteAll(d->m_eventDescriptions.values());
    d->m_eventDescriptions.clear();

    qDeleteAll(d->m_v8EventList);
    d->m_v8EventList.clear();

    d->m_endTimeSortedList.clear();
    d->m_startTimeSortedList.clear();

    d->m_v8parents.clear();

    d->clearQmlRootEvent();
    d->clearV8RootEvent();

    foreach (QmlEventTypeCount *typeCount, d->m_typeCounts.values())
        delete typeCount;
    d->m_typeCounts.clear();

    d->m_traceEndTime = 0;
    d->m_traceStartTime = -1;
    d->m_qmlMeasuredTime = 0;
    d->m_v8MeasuredTime = 0;

    emit countChanged();
    emit dataClear();
}

QList <QmlEventData *> QmlProfilerEventList::getEventDescriptions() const
{
    return d->m_eventDescriptions.values();
}

QmlEventData *QmlProfilerEventList::eventDescription(int eventId) const
{
    foreach (QmlEventData *event, d->m_eventDescriptions.values()) {
        if (event->eventId == eventId)
            return event;
    }
    return 0;
}

QV8EventData *QmlProfilerEventList::v8EventDescription(int eventId) const
{
    foreach (QV8EventData *event, d->m_v8EventList) {
        if (event->eventId == eventId)
            return event;
    }
    return 0;
}

const QV8EventDescriptions& QmlProfilerEventList::getV8Events() const
{
    return d->m_v8EventList;
}

void QmlProfilerEventList::addRangedEvent(int type, qint64 startTime, qint64 length,
                                                const QStringList &data, const QString &fileName, int line)
{
    const QChar colon = QLatin1Char(':');
    QString displayName, eventHashStr, details;

    emit processingData();

    // generate details string
    if (data.isEmpty())
        details = tr("Source code not available");
    else {
        details = data.join(" ").replace('\n'," ").simplified();
        QRegExp rewrite("\\(function \\$(\\w+)\\(\\) \\{ (return |)(.+) \\}\\)");
        bool match = rewrite.exactMatch(details);
        if (match) {
            details = rewrite.cap(1) + ": " + rewrite.cap(3);
        }
        if (details.startsWith(QString("file://")))
            details = details.mid(details.lastIndexOf(QChar('/')) + 1);
    }

    // generate hash
    if (fileName.isEmpty()) {
        displayName = tr("<bytecode>");
        eventHashStr = QString("--:%1:%2").arg(QString::number(type), details);
    } else {
        const QString filePath = QUrl(fileName).path();
        displayName = filePath.mid(filePath.lastIndexOf(QChar('/')) + 1) + colon + QString::number(line);
        eventHashStr = QString("%1:%2:%3:%4").arg(fileName, QString::number(line), QString::number(type), details);
    }

    QmlEventData *newEvent;
    if (d->m_eventDescriptions.contains(eventHashStr)) {
        newEvent = d->m_eventDescriptions[eventHashStr];
    } else {
        newEvent = new QmlEventData;
        newEvent->displayname = displayName;
        newEvent->filename = fileName;
        newEvent->eventHashStr = eventHashStr;
        newEvent->line = line;
        newEvent->eventType = (QmlJsDebugClient::QmlEventType)type;
        newEvent->details = details;
        d->m_eventDescriptions.insert(eventHashStr, newEvent);
    }

    QmlEventEndTimeData endTimeData;
    endTimeData.endTime = startTime + length;
    endTimeData.description = newEvent;
    endTimeData.startTimeIndex = d->m_startTimeSortedList.count();

    QmlEventStartTimeData startTimeData;
    startTimeData.startTime = startTime;
    startTimeData.length = length;
    startTimeData.description = newEvent;
    startTimeData.endTimeIndex = d->m_endTimeSortedList.count();

    d->m_endTimeSortedList << endTimeData;
    d->m_startTimeSortedList << startTimeData;

    emit countChanged();
}

void QmlProfilerEventList::addV8Event(int depth, const QString &function, const QString &filename, int lineNumber, double totalTime, double selfTime)
{
    QString displayName = filename.mid(filename.lastIndexOf(QLatin1Char('/')) + 1) + QLatin1Char(':') + QString::number(lineNumber);
    QV8EventData *eventData = 0;

    // time is given in milliseconds, but internally we store it in microseconds
    totalTime *= 1e6;
    selfTime *= 1e6;

    // cumulate information
    foreach (QV8EventData *v8event, d->m_v8EventList) {
        if (v8event->displayName == displayName && v8event->functionName == function) {
            eventData = v8event;
            break;
        }
    }

    if (!eventData) {
        eventData = new QV8EventData;
        eventData->displayName = displayName;
        eventData->filename = filename;
        eventData->functionName = function;
        eventData->line = lineNumber;
        eventData->totalTime = totalTime;
        eventData->selfTime = selfTime;
        d->m_v8EventList << eventData;
    } else {
        eventData->totalTime += totalTime;
        eventData->selfTime += selfTime;
    }
    d->m_v8parents[depth] = eventData;

    QV8EventData *parentEvent = 0;
    if (depth == 0) {
        parentEvent = &d->m_v8RootEvent;
        d->m_v8MeasuredTime += totalTime;
    }
    if (depth > 0 && d->m_v8parents.contains(depth-1)) {
        parentEvent = d->m_v8parents.value(depth-1);
    }

    if (parentEvent != 0) {
        if (!eventData->parentHash.contains(parentEvent->displayName)) {
            QV8EventSub *newParentSub = new QV8EventSub(parentEvent);
            newParentSub->totalTime = totalTime;

            eventData->parentHash.insert(parentEvent->displayName, newParentSub );
        } else {
            QV8EventSub *newParentSub = eventData->parentHash.value(parentEvent->displayName);
            newParentSub->totalTime += totalTime;
        }

        if (!parentEvent->childrenHash.contains(eventData->displayName)) {
            QV8EventSub *newChildSub = new QV8EventSub(eventData);
            newChildSub->totalTime = totalTime;

            parentEvent->childrenHash.insert(eventData->displayName, newChildSub);
        } else {
            QV8EventSub *newChildSub = parentEvent->childrenHash.value(eventData->displayName);
            newChildSub->totalTime += totalTime;
        }
    }
}

void QmlProfilerEventList::QmlProfilerEventListPrivate::collectV8Statistics()
{
    if (!m_v8EventList.isEmpty()) {
        double totalTimes = m_v8MeasuredTime;
        double selfTimes = 0;
        foreach (QV8EventData *v8event, m_v8EventList) {
            selfTimes += v8event->selfTime;
        }

        // prevent divisions by 0
        if (totalTimes == 0)
            totalTimes = 1;
        if (selfTimes == 0)
            selfTimes = 1;

        // insert root event in eventlist
        // the +1 ns is to get it on top of the sorted list
        m_v8RootEvent.totalTime = m_v8MeasuredTime + 1;
        m_v8RootEvent.selfTime = 0;

        int rootEventIndex = -1;
        for (int ndx = 0; ndx < m_v8EventList.count(); ndx++)
        {
            if (m_v8EventList.at(ndx)->displayName == m_rootEventName) {
                m_v8RootEvent = *m_v8EventList.at(ndx);
                rootEventIndex = ndx;
                break;
            }
        }
        if (rootEventIndex == -1) {
            rootEventIndex = m_v8EventList.count();
            QV8EventData *newRootEvent = new QV8EventData;
            *newRootEvent = m_v8RootEvent;
            m_v8EventList << newRootEvent;
        }

        foreach (QV8EventData *v8event, m_v8EventList) {
            v8event->totalPercent = v8event->totalTime * 100.0 / totalTimes;
            v8event->selfPercent = v8event->selfTime * 100.0 / selfTimes;
        }

        int index = 0;
        foreach (QV8EventData *v8event, m_v8EventList) {
            v8event->eventId = index++;
        }
        m_v8RootEvent.eventId = m_v8EventList[rootEventIndex]->eventId;
    }
}

void QmlProfilerEventList::setTraceEndTime( qint64 time )
{
    d->m_traceEndTime = time;
}

void QmlProfilerEventList::setTraceStartTime( qint64 time )
{
    d->m_traceStartTime = time;
}

void QmlProfilerEventList::complete()
{
    emit postProcessing();
    d->collectV8Statistics();
    postProcess();
}

void QmlProfilerEventList::QmlProfilerEventListPrivate::clearQmlRootEvent()
{
    m_qmlRootEvent.displayname = m_rootEventName;
    m_qmlRootEvent.filename = QString();
    m_qmlRootEvent.eventHashStr = m_rootEventName;
    m_qmlRootEvent.details = m_rootEventDesc;
    m_qmlRootEvent.line = 01;
    m_qmlRootEvent.eventType = QmlJsDebugClient::Binding;
    m_qmlRootEvent.duration = 0;
    m_qmlRootEvent.calls = 0;
    m_qmlRootEvent.minTime = 0;
    m_qmlRootEvent.maxTime = 0;
    m_qmlRootEvent.timePerCall = 0;
    m_qmlRootEvent.percentOfTime = 0;
    m_qmlRootEvent.medianTime = 0;
    m_qmlRootEvent.eventId = -1;

    qDeleteAll(m_qmlRootEvent.parentHash.values());
    qDeleteAll(m_qmlRootEvent.childrenHash.values());
    m_qmlRootEvent.parentHash.clear();
    m_qmlRootEvent.childrenHash.clear();
}

void QmlProfilerEventList::QmlProfilerEventListPrivate::clearV8RootEvent()
{
    m_v8RootEvent.displayName = m_rootEventName;
    m_v8RootEvent.functionName = m_rootEventDesc;
    m_v8RootEvent.line = -1;
    m_v8RootEvent.totalTime = 0;
    m_v8RootEvent.totalPercent = 0;
    m_v8RootEvent.selfTime = 0;
    m_v8RootEvent.selfPercent = 0;
    m_v8RootEvent.eventId = -1;

    qDeleteAll(m_v8RootEvent.parentHash.values());
    qDeleteAll(m_v8RootEvent.childrenHash.values());
    m_v8RootEvent.parentHash.clear();
    m_v8RootEvent.childrenHash.clear();
}

void QmlProfilerEventList::compileStatistics(qint64 startTime, qint64 endTime)
{
    int index;
    int fromIndex = findFirstIndex(startTime);
    int toIndex = findLastIndex(endTime);
    double totalTime = 0;

    // clear existing statistics
    foreach (QmlEventData *eventDescription, d->m_eventDescriptions.values()) {
        eventDescription->calls = 0;
        // maximum possible value
        eventDescription->minTime = d->m_endTimeSortedList.last().endTime;
        eventDescription->maxTime = 0;
        eventDescription->medianTime = 0;
        eventDescription->duration = 0;
        qDeleteAll(eventDescription->parentHash);
        qDeleteAll(eventDescription->childrenHash);
        eventDescription->parentHash.clear();
        eventDescription->childrenHash.clear();
    }

    // create root event for statistics
    d->clearQmlRootEvent();

    // compute parent-child relationship and call count
    QHash<int, QmlEventData*> lastParent;
    for (index = fromIndex; index <= toIndex; index++) {
        QmlEventData *eventDescription = d->m_startTimeSortedList[index].description;

        if (d->m_startTimeSortedList[index].startTime > endTime ||
                d->m_startTimeSortedList[index].startTime+d->m_startTimeSortedList[index].length < startTime) {
            continue;
        }

        eventDescription->calls++;
        qint64 duration = d->m_startTimeSortedList[index].length;
        eventDescription->duration += duration;
        if (eventDescription->maxTime < duration)
            eventDescription->maxTime = duration;
        if (eventDescription->minTime > duration)
            eventDescription->minTime = duration;

        int level = d->m_startTimeSortedList[index].level;

        QmlEventData *parentEvent = &d->m_qmlRootEvent;
        if (level > MIN_LEVEL && lastParent.contains(level-1)) {
            parentEvent = lastParent[level-1];
        }

        if (!eventDescription->parentHash.contains(parentEvent->eventHashStr)) {
            QmlEventSub *newParentEvent = new QmlEventSub(parentEvent);
            newParentEvent->calls = 1;
            newParentEvent->duration = duration;

            eventDescription->parentHash.insert(parentEvent->eventHashStr, newParentEvent);
        } else {
            QmlEventSub *newParentEvent = eventDescription->parentHash.value(parentEvent->eventHashStr);
            newParentEvent->duration += duration;
            newParentEvent->calls++;
        }

        if (!parentEvent->childrenHash.contains(eventDescription->eventHashStr)) {
            QmlEventSub *newChildEvent = new QmlEventSub(eventDescription);
            newChildEvent->calls = 1;
            newChildEvent->duration = duration;

            parentEvent->childrenHash.insert(eventDescription->eventHashStr, newChildEvent);
        } else {
            QmlEventSub *newChildEvent = parentEvent->childrenHash.value(eventDescription->eventHashStr);
            newChildEvent->duration += duration;
            newChildEvent->calls++;
        }

        lastParent[level] = eventDescription;

        if (level == MIN_LEVEL) {
            totalTime += duration;
        }
    }

    // fake rootEvent statistics
    // the +1 nanosecond is to force it to be on top of the sorted list
    d->m_qmlRootEvent.duration = totalTime+1;
    d->m_qmlRootEvent.minTime = totalTime+1;
    d->m_qmlRootEvent.maxTime = totalTime+1;
    d->m_qmlRootEvent.medianTime = totalTime+1;
    if (totalTime > 0)
        d->m_qmlRootEvent.calls = 1;

    // insert into list
    QmlEventData *listedRootEvent = d->m_eventDescriptions.value(d->m_rootEventName);
    if (!listedRootEvent) {
        listedRootEvent = new QmlEventData;
        d->m_eventDescriptions.insert(d->m_rootEventName, listedRootEvent);
    }
    *listedRootEvent = d->m_qmlRootEvent;

    // compute percentages
    foreach (QmlEventData *binding, d->m_eventDescriptions.values()) {
        binding->percentOfTime = binding->duration * 100.0 / totalTime;
        binding->timePerCall = binding->calls > 0 ? double(binding->duration) / binding->calls : 0;
    }

    // compute median time
    QHash < QmlEventData* , QList<qint64> > durationLists;
    for (index = fromIndex; index <= toIndex; index++) {
        QmlEventData *desc = d->m_startTimeSortedList[index].description;
        qint64 len = d->m_startTimeSortedList[index].length;
        durationLists[desc].append(len);
    }
    QMutableHashIterator < QmlEventData* , QList<qint64> > iter(durationLists);
    while (iter.hasNext()) {
        iter.next();
        if (!iter.value().isEmpty()) {
            qSort(iter.value());
            iter.key()->medianTime = iter.value().at(iter.value().count()/2);
        }
    }
}

void QmlProfilerEventList::prepareForDisplay()
{
    // generate numeric ids
    int ndx = 0;
    foreach (QmlEventData *binding, d->m_eventDescriptions.values()) {
        binding->eventId = ndx++;
    }

    // collect type counts
    foreach (const QmlEventStartTimeData &eventStartData, d->m_startTimeSortedList) {
        int typeNumber = eventStartData.description->eventType;
        if (!d->m_typeCounts.contains(typeNumber)) {
            d->m_typeCounts[typeNumber] = new QmlEventTypeCount;
            d->m_typeCounts[typeNumber]->nestingCount = 0;
        }
        if (eventStartData.nestingLevel > d->m_typeCounts[typeNumber]->nestingCount) {
            d->m_typeCounts[typeNumber]->nestingCount = eventStartData.nestingLevel;
        }
        if (!d->m_typeCounts[typeNumber]->eventIds.contains(eventStartData.description->eventId))
            d->m_typeCounts[typeNumber]->eventIds << eventStartData.description->eventId;
    }
}

void QmlProfilerEventList::sortStartTimes()
{
    if (d->m_startTimeSortedList.count() < 2)
        return;

    // assuming startTimes is partially sorted
    // identify blocks of events and sort them with quicksort
    QList<QmlEventStartTimeData>::iterator itFrom = d->m_startTimeSortedList.end() - 2;
    QList<QmlEventStartTimeData>::iterator itTo = d->m_startTimeSortedList.end() - 1;

    while (itFrom != d->m_startTimeSortedList.begin() && itTo != d->m_startTimeSortedList.begin()) {
        // find block to sort
        while ( itFrom != d->m_startTimeSortedList.begin()
                && itTo->startTime > itFrom->startTime ) {
            itTo--;
            itFrom = itTo - 1;
        }

        // if we're at the end of the list
        if (itFrom == d->m_startTimeSortedList.begin())
            break;

        // find block length
        while ( itFrom != d->m_startTimeSortedList.begin()
                && itTo->startTime <= itFrom->startTime )
            itFrom--;

        if (itTo->startTime <= itFrom->startTime)
            qSort(itFrom, itTo + 1, compareStartTimes);
        else
            qSort(itFrom + 1, itTo + 1, compareStartTimes);

        // move to next block
        itTo = itFrom;
        itFrom = itTo - 1;
    }

    // link back the endTimes
    for (int i = 0; i < d->m_startTimeSortedList.length(); i++)
       d->m_endTimeSortedList[d->m_startTimeSortedList[i].endTimeIndex].startTimeIndex = i;
}

void QmlProfilerEventList::sortEndTimes()
{
    // assuming endTimes is partially sorted
    // identify blocks of events and sort them with quicksort

    if (d->m_endTimeSortedList.count() < 2)
        return;

    QList<QmlEventEndTimeData>::iterator itFrom = d->m_endTimeSortedList.begin();
    QList<QmlEventEndTimeData>::iterator itTo = d->m_endTimeSortedList.begin() + 1;

    while (itTo != d->m_endTimeSortedList.end() && itFrom != d->m_endTimeSortedList.end()) {
        // find block to sort
        while ( itTo != d->m_endTimeSortedList.end()
                && d->m_startTimeSortedList[itTo->startTimeIndex].startTime >
                d->m_startTimeSortedList[itFrom->startTimeIndex].startTime +
                d->m_startTimeSortedList[itFrom->startTimeIndex].length ) {
            itFrom++;
            itTo = itFrom+1;
        }

        // if we're at the end of the list
        if (itTo == d->m_endTimeSortedList.end())
            break;

        // find block length
        while ( itTo != d->m_endTimeSortedList.end()
                && d->m_startTimeSortedList[itTo->startTimeIndex].startTime <=
                d->m_startTimeSortedList[itFrom->startTimeIndex].startTime +
                d->m_startTimeSortedList[itFrom->startTimeIndex].length )
            itTo++;

        // sort block
        qSort(itFrom, itTo, compareEndTimes);

        // move to next block
        itFrom = itTo;
        itTo = itFrom+1;

    }

    // link back the startTimes
    for (int i = 0; i < d->m_endTimeSortedList.length(); i++)
        d->m_startTimeSortedList[d->m_endTimeSortedList[i].startTimeIndex].endTimeIndex = i;
}

void QmlProfilerEventList::computeNestingLevels()
{
    // compute levels
    QHash <int, qint64> endtimesPerLevel;
    QList <int> nestingLevels;
    QList < QHash <int, qint64> > endtimesPerNestingLevel;
    int level = MIN_LEVEL;
    endtimesPerLevel[MIN_LEVEL] = 0;

    for (int i = 0; i < QmlJsDebugClient::MaximumQmlEventType; i++) {
        nestingLevels << MIN_LEVEL;
        QHash <int, qint64> dummyHash;
        dummyHash[MIN_LEVEL] = 0;
        endtimesPerNestingLevel << dummyHash;
    }

    for (int i=0; i<d->m_startTimeSortedList.count(); i++) {
        qint64 st = d->m_startTimeSortedList[i].startTime;
        int type = d->m_startTimeSortedList[i].description->eventType;

        // general level
        if (endtimesPerLevel[level] > st) {
            level++;
        } else {
            while (level > MIN_LEVEL && endtimesPerLevel[level-1] <= st)
                level--;
        }
        endtimesPerLevel[level] = st + d->m_startTimeSortedList[i].length;

        // per type
        if (endtimesPerNestingLevel[type][nestingLevels[type]] > st) {
            nestingLevels[type]++;
        } else {
            while (nestingLevels[type] > MIN_LEVEL &&
                   endtimesPerNestingLevel[type][nestingLevels[type]-1] <= st)
                nestingLevels[type]--;
        }
        endtimesPerNestingLevel[type][nestingLevels[type]] = st + d->m_startTimeSortedList[i].length;

        d->m_startTimeSortedList[i].level = level;
        d->m_startTimeSortedList[i].nestingLevel = nestingLevels[type];

        if (level == MIN_LEVEL) {
            d->m_qmlMeasuredTime += d->m_startTimeSortedList[i].length;
        }
    }
}

void QmlProfilerEventList::computeNestingDepth()
{
    QHash <int, int> nestingDepth;
    for (int i = 0; i < d->m_endTimeSortedList.count(); i++) {
        int type = d->m_endTimeSortedList[i].description->eventType;
        int nestingInType = d->m_startTimeSortedList[ d->m_endTimeSortedList[i].startTimeIndex ].nestingLevel;
        if (!nestingDepth.contains(type))
            nestingDepth[type] = nestingInType;
        else {
            int nd = nestingDepth[type];
            nestingDepth[type] = nd > nestingInType ? nd : nestingInType;
        }

        d->m_startTimeSortedList[ d->m_endTimeSortedList[i].startTimeIndex ].nestingDepth = nestingDepth[type];
        if (nestingInType == MIN_LEVEL)
            nestingDepth[type] = MIN_LEVEL;
    }
}

void QmlProfilerEventList::postProcess()
{
    if (count() != 0) {
        sortStartTimes();
        sortEndTimes();
        computeLevels();
        linkEndsToStarts();
        prepareForDisplay();
        compileStatistics(traceStartTime(), traceEndTime());
    }
    // data is ready even when there's no data
    emit dataReady();
}

void QmlProfilerEventList::linkEndsToStarts()
{
    for (int i = 0; i < d->m_startTimeSortedList.count(); i++)
        d->m_endTimeSortedList[d->m_startTimeSortedList[i].endTimeIndex].startTimeIndex = i;
}

void QmlProfilerEventList::computeLevels()
{
    computeNestingLevels();
    computeNestingDepth();
}

// get list of events between A and B:
// find fist event with endtime after A -> aa
// find last event with starttime before B -> bb
// list is from parent of aa with level=0 to bb, in the "sorted by starttime" list
int QmlProfilerEventList::findFirstIndex(qint64 startTime) const
{
    int candidate = -1;
    // in the "endtime" list, find the first event that ends after startTime
    if (d->m_endTimeSortedList.isEmpty())
        return 0; // -1
    if (d->m_endTimeSortedList.length() == 1 || d->m_endTimeSortedList.first().endTime >= startTime)
        candidate = 0;
    else
        if (d->m_endTimeSortedList.last().endTime <= startTime)
            return 0; // -1

    if (candidate == -1)
    {
        int fromIndex = 0;
        int toIndex = d->m_endTimeSortedList.count()-1;
        while (toIndex - fromIndex > 1) {
            int midIndex = (fromIndex + toIndex)/2;
            if (d->m_endTimeSortedList[midIndex].endTime < startTime)
                fromIndex = midIndex;
            else
                toIndex = midIndex;
        }

        candidate = toIndex;
    }

    int ndx = d->m_endTimeSortedList[candidate].startTimeIndex;

    // and then go to the parent
    while (d->m_startTimeSortedList[ndx].level != MIN_LEVEL && ndx > 0)
        ndx--;

    return ndx;
}

int QmlProfilerEventList::findFirstIndexNoParents(qint64 startTime) const
{
    int candidate = -1;
    // in the "endtime" list, find the first event that ends after startTime
    if (d->m_endTimeSortedList.isEmpty())
        return 0; // -1
    if (d->m_endTimeSortedList.length() == 1 || d->m_endTimeSortedList.first().endTime >= startTime)
        candidate = 0;
    else
        if (d->m_endTimeSortedList.last().endTime <= startTime)
            return 0; // -1

    if (candidate == -1) {
        int fromIndex = 0;
        int toIndex = d->m_endTimeSortedList.count()-1;
        while (toIndex - fromIndex > 1) {
            int midIndex = (fromIndex + toIndex)/2;
            if (d->m_endTimeSortedList[midIndex].endTime < startTime)
                fromIndex = midIndex;
            else
                toIndex = midIndex;
        }

        candidate = toIndex;
    }

    int ndx = d->m_endTimeSortedList[candidate].startTimeIndex;

    return ndx;
}

int QmlProfilerEventList::findLastIndex(qint64 endTime) const
{
    // in the "starttime" list, find the last event that starts before endtime
    if (d->m_startTimeSortedList.isEmpty())
        return 0; // -1
    if (d->m_startTimeSortedList.first().startTime >= endTime)
        return 0; // -1
    if (d->m_startTimeSortedList.length() == 1)
        return 0;
    if (d->m_startTimeSortedList.last().startTime <= endTime)
        return d->m_startTimeSortedList.count()-1;

    int fromIndex = 0;
    int toIndex = d->m_startTimeSortedList.count()-1;
    while (toIndex - fromIndex > 1) {
        int midIndex = (fromIndex + toIndex)/2;
        if (d->m_startTimeSortedList[midIndex].startTime < endTime)
            fromIndex = midIndex;
        else
            toIndex = midIndex;
    }

    return fromIndex;
}

qint64 QmlProfilerEventList::firstTimeMark() const
{
    if (d->m_startTimeSortedList.isEmpty())
        return 0;
    else {
        return d->m_startTimeSortedList[0].startTime;
    }
}

qint64 QmlProfilerEventList::lastTimeMark() const
{
    if (d->m_endTimeSortedList.isEmpty())
        return 0;
    else {
        return d->m_endTimeSortedList.last().endTime;
    }
}

qint64 QmlProfilerEventList::traceStartTime() const
{
    return d->m_traceStartTime != -1? d->m_traceStartTime : firstTimeMark();
}

qint64 QmlProfilerEventList::traceEndTime() const
{
    return d->m_traceEndTime ? d->m_traceEndTime : lastTimeMark();
}

qint64 QmlProfilerEventList::traceDuration() const
{
    return traceEndTime() - traceStartTime();
}

qint64 QmlProfilerEventList::qmlMeasuredTime() const
{
    return d->m_qmlMeasuredTime;
}
qint64 QmlProfilerEventList::v8MeasuredTime() const
{
    return d->m_v8MeasuredTime;
}

int QmlProfilerEventList::count() const
{
    return d->m_startTimeSortedList.count();
}

////////////////////////////////////////////////////////////////////////////////////


bool QmlProfilerEventList::save(const QString &filename)
{
    if (count() == 0) {
        emit error(tr("No data to save"));
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        emit error(tr("Could not open %1 for writing").arg(filename));
        return false;
    }

    QXmlStreamWriter stream(&file);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    stream.writeStartElement("trace");
    stream.writeAttribute("version", Constants::PROFILER_FILE_VERSION);

    stream.writeAttribute("traceStart", QString::number(traceStartTime()));
    stream.writeAttribute("traceEnd", QString::number(traceEndTime()));

    stream.writeStartElement("eventData");
    stream.writeAttribute("totalTime", QString::number(d->m_qmlMeasuredTime));

    foreach (const QmlEventData *eventData, d->m_eventDescriptions.values()) {
        stream.writeStartElement("event");
        stream.writeAttribute("index", QString::number(d->m_eventDescriptions.keys().indexOf(eventData->eventHashStr)));
        stream.writeTextElement("displayname", eventData->displayname);
        stream.writeTextElement("type", qmlEventType(eventData->eventType));
        if (!eventData->filename.isEmpty()) {
            stream.writeTextElement("filename", eventData->filename);
            stream.writeTextElement("line", QString::number(eventData->line));
        }
        stream.writeTextElement("details", eventData->details);
        stream.writeEndElement();
    }
    stream.writeEndElement(); // eventData

    stream.writeStartElement("eventList");
    foreach (const QmlEventStartTimeData &rangedEvent, d->m_startTimeSortedList) {
        stream.writeStartElement("range");
        stream.writeAttribute("startTime", QString::number(rangedEvent.startTime));
        stream.writeAttribute("duration", QString::number(rangedEvent.length));
        stream.writeAttribute("eventIndex", QString::number(d->m_eventDescriptions.keys().indexOf(rangedEvent.description->eventHashStr)));
        stream.writeEndElement();
    }
    stream.writeEndElement(); // eventList

    stream.writeStartElement("v8profile"); // v8 profiler output
    stream.writeAttribute("totalTime", QString::number(d->m_v8MeasuredTime));
    foreach (QV8EventData *v8event, d->m_v8EventList) {
        stream.writeStartElement("event");
        stream.writeAttribute("index", QString::number(d->m_v8EventList.indexOf(v8event)));
        stream.writeTextElement("displayname", v8event->displayName);
        stream.writeTextElement("functionname", v8event->functionName);
        if (!v8event->filename.isEmpty()) {
            stream.writeTextElement("filename", v8event->filename);
            stream.writeTextElement("line", QString::number(v8event->line));
        }
        stream.writeTextElement("totalTime", QString::number(v8event->totalTime));
        stream.writeTextElement("selfTime", QString::number(v8event->selfTime));
        if (!v8event->childrenHash.isEmpty()) {
            stream.writeStartElement("childrenEvents");
            QStringList childrenIndexes;
            QStringList childrenTimes;
            QStringList parentTimes;
            foreach (QV8EventSub *v8child, v8event->childrenHash.values()) {
                childrenIndexes << QString::number(v8child->reference->eventId);
                childrenTimes << QString::number(v8child->totalTime);
                parentTimes << QString::number(d->m_v8EventList[v8child->reference->eventId]->parentHash[v8event->displayName]->totalTime);
            }

            stream.writeAttribute("list", childrenIndexes.join(QString(", ")));
            stream.writeAttribute("childrenTimes", childrenTimes.join(QString(", ")));
            stream.writeAttribute("parentTimes", parentTimes.join(QString(", ")));
            stream.writeEndElement();
        }
        stream.writeEndElement();
    }
    stream.writeEndElement(); // v8 profiler output

    stream.writeEndElement(); // trace
    stream.writeEndDocument();

    file.close();
    return true;
}

void QmlProfilerEventList::setFilename(const QString &filename)
{
    d->m_filename = filename;
}

void QmlProfilerEventList::load(const QString &filename)
{
    setFilename(filename);
    load();
}

// "be strict in your output but tolerant in your inputs"
void QmlProfilerEventList::load()
{
    QString filename = d->m_filename;

    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit error(tr("Could not open %1 for reading").arg(filename));
        return;
    }

    emit processingData();

    // erase current
    clear();

    bool readingQmlEvents = false;
    bool readingV8Events = false;
    QHash <int, QmlEventData *> descriptionBuffer;
    QmlEventData *currentEvent = 0;
    QHash <int, QV8EventData *> v8eventBuffer;
    QHash <int, QString> childrenIndexes;
    QHash <int, QString> childrenTimes;
    QHash <int, QString> parentTimes;
    QV8EventData *v8event = 0;
    bool startTimesAreSorted = true;
    bool validVersion = true;

    // time computation
    d->m_v8MeasuredTime = 0;
    d->m_qmlMeasuredTime = 0;
    double cumulatedV8Time = 0;

    QXmlStreamReader stream(&file);

    while (validVersion && !stream.atEnd() && !stream.hasError()) {
        QXmlStreamReader::TokenType token = stream.readNext();
        QString elementName = stream.name().toString();
        switch (token) {
            case QXmlStreamReader::StartDocument :  continue;
            case QXmlStreamReader::StartElement : {
                if (elementName == "trace") {
                    QXmlStreamAttributes attributes = stream.attributes();
                    if (attributes.hasAttribute("version"))
                        validVersion = attributes.value("version").toString() == Constants::PROFILER_FILE_VERSION;
                    else
                        validVersion = false;
                    if (attributes.hasAttribute("traceStart"))
                        setTraceStartTime(attributes.value("traceStart").toString().toLongLong());
                    if (attributes.hasAttribute("traceEnd"))
                        setTraceEndTime(attributes.value("traceEnd").toString().toLongLong());
                }
                if (elementName == "eventData" && !readingV8Events) {
                    readingQmlEvents = true;
                    QXmlStreamAttributes attributes = stream.attributes();
                    if (attributes.hasAttribute("totalTime"))
                        d->m_qmlMeasuredTime = attributes.value("totalTime").toString().toDouble();
                    break;
                }
                if (elementName == "v8profile" && !readingQmlEvents) {
                    readingV8Events = true;
                    QXmlStreamAttributes attributes = stream.attributes();
                    if (attributes.hasAttribute("totalTime"))
                        d->m_v8MeasuredTime = attributes.value("totalTime").toString().toDouble();
                    break;
                }

                if (elementName == "trace") {
                    QXmlStreamAttributes attributes = stream.attributes();
                    if (attributes.hasAttribute("traceStart"))
                        setTraceStartTime(attributes.value("traceStart").toString().toLongLong());
                    if (attributes.hasAttribute("traceEnd"))
                        setTraceEndTime(attributes.value("traceEnd").toString().toLongLong());
                }

                if (elementName == "range") {
                    QmlEventStartTimeData rangedEvent;
                    QXmlStreamAttributes attributes = stream.attributes();
                    if (attributes.hasAttribute("startTime"))
                        rangedEvent.startTime = attributes.value("startTime").toString().toLongLong();
                    if (attributes.hasAttribute("duration"))
                        rangedEvent.length = attributes.value("duration").toString().toLongLong();
                    if (attributes.hasAttribute("eventIndex")) {
                        int ndx = attributes.value("eventIndex").toString().toInt();
                        if (!descriptionBuffer.value(ndx))
                            descriptionBuffer[ndx] = new QmlEventData;
                        rangedEvent.description = descriptionBuffer.value(ndx);
                    }
                    rangedEvent.endTimeIndex = d->m_endTimeSortedList.length();

                    if (!d->m_startTimeSortedList.isEmpty()
                            && rangedEvent.startTime < d->m_startTimeSortedList.last().startTime)
                        startTimesAreSorted = false;
                    d->m_startTimeSortedList << rangedEvent;

                    QmlEventEndTimeData endTimeEvent;
                    endTimeEvent.endTime = rangedEvent.startTime + rangedEvent.length;
                    endTimeEvent.startTimeIndex = d->m_startTimeSortedList.length()-1;
                    endTimeEvent.description = rangedEvent.description;
                    d->m_endTimeSortedList << endTimeEvent;
                    break;
                }

                if (readingQmlEvents) {
                    if (elementName == "event") {
                        QXmlStreamAttributes attributes = stream.attributes();
                        if (attributes.hasAttribute("index")) {
                            int ndx = attributes.value("index").toString().toInt();
                            if (!descriptionBuffer.value(ndx))
                                descriptionBuffer[ndx] = new QmlEventData;
                            currentEvent = descriptionBuffer[ndx];
                        } else {
                            currentEvent = 0;
                        }
                        break;
                    }

                    // the remaining are eventdata or v8eventdata elements
                    if (!currentEvent)
                        break;

                    stream.readNext();
                    if (stream.tokenType() != QXmlStreamReader::Characters)
                        break;
                    QString readData = stream.text().toString();

                    if (elementName == "displayname") {
                        currentEvent->displayname = readData;
                        break;
                    }
                    if (elementName == "type") {
                        currentEvent->eventType = qmlEventType(readData);
                        break;
                    }
                    if (elementName == "filename") {
                        currentEvent->filename = readData;
                        break;
                    }
                    if (elementName == "line") {
                        currentEvent->line = readData.toInt();
                        break;
                    }
                    if (elementName == "details") {
                        currentEvent->details = readData;
                        break;
                    }
                }

                if (readingV8Events) {
                    if (elementName == "event") {
                        QXmlStreamAttributes attributes = stream.attributes();
                        if (attributes.hasAttribute("index")) {
                            int ndx = attributes.value("index").toString().toInt();
                            if (!v8eventBuffer.value(ndx))
                                v8eventBuffer[ndx] = new QV8EventData;
                            v8event = v8eventBuffer[ndx];
                        } else {
                            v8event = 0;
                        }
                        break;
                    }

                    // the remaining are eventdata or v8eventdata elements
                    if (!v8event)
                        break;

                    if (elementName == "childrenEvents") {
                        QXmlStreamAttributes attributes = stream.attributes();
                        int eventIndex = v8eventBuffer.key(v8event);
                        if (attributes.hasAttribute("list")) {
                            // store for later parsing (we haven't read all the events yet)
                            childrenIndexes[eventIndex] = attributes.value("list").toString();
                        }
                        if (attributes.hasAttribute("childrenTimes")) {
                            childrenTimes[eventIndex] = attributes.value("childrenTimes").toString();
                        }
                        if (attributes.hasAttribute("parentTimes")) {
                            parentTimes[eventIndex] = attributes.value("parentTimes").toString();
                        }
                    }

                    stream.readNext();
                    if (stream.tokenType() != QXmlStreamReader::Characters)
                        break;
                    QString readData = stream.text().toString();

                    if (elementName == "displayname") {
                        v8event->displayName = readData;
                        break;
                    }

                    if (elementName == "functionname") {
                        v8event->functionName = readData;
                        break;
                    }

                    if (elementName == "filename") {
                        v8event->filename = readData;
                        break;
                    }

                    if (elementName == "line") {
                        v8event->line = readData.toInt();
                        break;
                    }

                    if (elementName == "totalTime") {
                        v8event->totalTime = readData.toDouble();
                        cumulatedV8Time += v8event->totalTime;
                        break;
                    }

                    if (elementName == "selfTime") {
                        v8event->selfTime = readData.toDouble();
                        break;
                    }
                }

                break;
            }
            case QXmlStreamReader::EndElement : {
                if (elementName == "event") {
                    currentEvent = 0;
                    break;
                }
                if (elementName == "eventData") {
                    readingQmlEvents = false;
                    break;
                }
                if (elementName == "v8profile") {
                    readingV8Events = false;
                }
            }
            default: break;
        }
    }

    file.close();

    if (stream.hasError()) {
        emit error(tr("Error while parsing %1").arg(filename));
        clear();
        return;
    }

    stream.clear();

    if (!validVersion) {
        clear();
        emit countChanged();
        emit dataReady();
        emit error(tr("Invalid version of QmlProfiler trace file."));
        return;
    }

    // backwards compatibility
    if (d->m_v8MeasuredTime == 0)
        d->m_v8MeasuredTime = cumulatedV8Time;

    // move the buffered data to the details cache
    foreach (QmlEventData *desc, descriptionBuffer.values()) {
        QString location = QString("%1:%2:%3").arg(QString::number(desc->eventType), desc->displayname, desc->details);
        desc->eventHashStr = location;
        d->m_eventDescriptions[location] = desc;
    }

    // sort startTimeSortedList
    if (!startTimesAreSorted) {
        qSort(d->m_startTimeSortedList.begin(), d->m_startTimeSortedList.end(), compareStartTimes);
        for (int i = 0; i< d->m_startTimeSortedList.length(); i++) {
            QmlEventStartTimeData startTimeData = d->m_startTimeSortedList[i];
            d->m_endTimeSortedList[startTimeData.endTimeIndex].startTimeIndex = i;
        }
        qSort(d->m_endTimeSortedList.begin(), d->m_endTimeSortedList.end(), compareStartIndexes);
    }

    // find v8events' children and parents
    foreach (int parentIndex, childrenIndexes.keys()) {
        QStringList childrenStrings = childrenIndexes.value(parentIndex).split(",");
        QStringList childrenTimesStrings = childrenTimes.value(parentIndex).split(", ");
        QStringList parentTimesStrings = parentTimes.value(parentIndex).split(", ");
        for (int ndx = 0; ndx < childrenStrings.count(); ndx++) {
            int childIndex = childrenStrings[ndx].toInt();
            if (v8eventBuffer.value(childIndex)) {
                QV8EventSub *newChild = new QV8EventSub(v8eventBuffer[childIndex]);
                QV8EventSub *newParent = new QV8EventSub(v8eventBuffer[parentIndex]);
                if (childrenTimesStrings.count() > ndx)
                    newChild->totalTime = childrenTimesStrings[ndx].toDouble();
                if (parentTimesStrings.count() > ndx)
                    newParent->totalTime = parentTimesStrings[ndx].toDouble();
                v8eventBuffer[parentIndex]->childrenHash.insert(newChild->reference->displayName, newChild);
                v8eventBuffer[childIndex]->parentHash.insert(newParent->reference->displayName, newParent);
            }
        }
    }
    // store v8 events
    d->m_v8EventList = v8eventBuffer.values();

    emit countChanged();

    descriptionBuffer.clear();

    emit postProcessing();
    d->collectV8Statistics();
    postProcess();
}

///////////////////////////////////////////////
qint64 QmlProfilerEventList::getStartTime(int index) const {
    return d->m_startTimeSortedList[index].startTime;
}

qint64 QmlProfilerEventList::getEndTime(int index) const {
    return d->m_startTimeSortedList[index].startTime + d->m_startTimeSortedList[index].length;
}

qint64 QmlProfilerEventList::getDuration(int index) const {
    return d->m_startTimeSortedList[index].length;
}

int QmlProfilerEventList::getType(int index) const {
    return d->m_startTimeSortedList[index].description->eventType;
}

int QmlProfilerEventList::getNestingLevel(int index) const {
    return d->m_startTimeSortedList[index].nestingLevel;
}

int QmlProfilerEventList::getNestingDepth(int index) const {
    return d->m_startTimeSortedList[index].nestingDepth;
}

QString QmlProfilerEventList::getFilename(int index) const {
    return d->m_startTimeSortedList[index].description->filename;
}

int QmlProfilerEventList::getLine(int index) const {
    return d->m_startTimeSortedList[index].description->line;
}

QString QmlProfilerEventList::getDetails(int index) const {
    return d->m_startTimeSortedList[index].description->details;
}

int QmlProfilerEventList::getEventId(int index) const {
    return d->m_startTimeSortedList[index].description->eventId;
}

int QmlProfilerEventList::uniqueEventsOfType(int type) const {
    if (!d->m_typeCounts.contains(type))
        return 0;
    return d->m_typeCounts[type]->eventIds.count();
}

int QmlProfilerEventList::maxNestingForType(int type) const {
    if (!d->m_typeCounts.contains(type))
        return 0;
    return d->m_typeCounts[type]->nestingCount;
}

QString QmlProfilerEventList::eventTextForType(int type, int index) const {
    if (!d->m_typeCounts.contains(type))
        return QString();
    return d->m_eventDescriptions.values().at(d->m_typeCounts[type]->eventIds[index])->details;
}

int QmlProfilerEventList::eventIdForType(int type, int index) const {
    if (!d->m_typeCounts.contains(type))
        return -1;
    return d->m_typeCounts[type]->eventIds[index];
}

int QmlProfilerEventList::eventPosInType(int index) const {
    int eventType = d->m_startTimeSortedList[index].description->eventType;
    return d->m_typeCounts[eventType]->eventIds.indexOf(d->m_startTimeSortedList[index].description->eventId);
}

} // namespace QmlJsDebugClient
