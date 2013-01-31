/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qmlprofilerdatamodel.h"

#include <QUrl>
#include <QHash>
#include <QtAlgorithms>
#include <QString>
#include <QStringList>

#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <QTimer>
#include <utils/qtcassert.h>

using namespace QmlDebug;

namespace QmlProfiler {
namespace Internal {

///////////////////////////////////////////////////////////
QmlRangeEventData::QmlRangeEventData()
{
    eventType = MaximumQmlEventType;
    eventId = -1;
    duration = 0;
    calls = 0;
    minTime = 0;
    maxTime = 0;
    timePerCall = 0;
    percentOfTime = 0;
    medianTime = 0;
    isBindingLoop = false;
}

QmlRangeEventData::~QmlRangeEventData()
{
    qDeleteAll(parentHash.values());
    parentHash.clear();
    qDeleteAll(childrenHash.values());
    childrenHash.clear();
}

QmlRangeEventData &QmlRangeEventData::operator=(const QmlRangeEventData &ref)
{
    if (this == &ref)
        return *this;

    displayName = ref.displayName;
    location = ref.location;
    eventHashStr = ref.eventHashStr;
    details = ref.details;
    eventType = ref.eventType;
    duration = ref.duration;
    calls = ref.calls;
    minTime = ref.minTime;
    maxTime = ref.maxTime;
    timePerCall = ref.timePerCall;
    percentOfTime = ref.percentOfTime;
    medianTime = ref.medianTime;
    eventId = ref.eventId;
    isBindingLoop = ref.isBindingLoop;

    qDeleteAll(parentHash.values());
    parentHash.clear();
    foreach (const QString &key, ref.parentHash.keys()) {
        parentHash.insert(key, new QmlRangeEventRelative(ref.parentHash.value(key)));
    }

    qDeleteAll(childrenHash.values());
    childrenHash.clear();
    foreach (const QString &key, ref.childrenHash.keys()) {
        childrenHash.insert(key, new QmlRangeEventRelative(ref.childrenHash.value(key)));
    }

    return *this;
}

///////////////////////////////////////////////////////////

// endtimedata
struct QmlRangeEventEndInstance {
    qint64 endTime;
    int startTimeIndex;
    QmlRangeEventData *description;
};

// starttimedata
struct QmlRangeEventStartInstance {
    qint64 startTime;
    qint64 duration;
    qint64 level;
    int endTimeIndex;
    qint64 nestingLevel;
    qint64 nestingDepth;
    QmlRangeEventData *statsInfo;

    int baseEventIndex;

    // animation-related data
    int frameRate;
    int animationCount;

    int bindingLoopHead;
};

struct QmlRangeEventTypeCount {
    QVector<int> eventIds;
    int nestingCount;
};

// used by quicksort
bool compareEndTimes(const QmlRangeEventEndInstance &t1, const QmlRangeEventEndInstance &t2)
{
    return t1.endTime < t2.endTime;
}

bool compareStartTimes(const QmlRangeEventStartInstance &t1, const QmlRangeEventStartInstance &t2)
{
    return t1.startTime < t2.startTime;
}

bool compareStartIndexes(const QmlRangeEventEndInstance &t1, const QmlRangeEventEndInstance &t2)
{
    return t1.startTimeIndex < t2.startTimeIndex;
}

//////////////////////////////////////////////////////////////
class QmlProfilerDataModel::QmlProfilerDataModelPrivate
{
public:
    QmlProfilerDataModelPrivate(QmlProfilerDataModel *qq) : q(qq) {}

    QmlProfilerDataModel *q;

    // convenience functions
    void clearQmlRootEvent();
    void insertQmlRootEvent();
    void postProcess();
    void sortEndTimes();
    void findAnimationLimits();
    void sortStartTimes();
    void computeNestingLevels();
    void computeNestingDepth();
    void prepareForDisplay();
    void linkStartsToEnds();
    void linkEndsToStarts();
    bool checkBindingLoop(QmlRangeEventData *from, QmlRangeEventData *current, QList<QmlRangeEventData *>visited);


    // stats
    void clearStatistics();
    void redoTree(qint64 fromTime, qint64 toTime);
    void computeMedianTime(qint64 fromTime, qint64 toTime);
    void findBindingLoops(qint64 fromTime, qint64 toTime);

    QmlProfilerDataModel::State listState;

    // Stored data
    QHash<QString, QmlRangeEventData *> rangeEventDictionary;
    QVector<QmlRangeEventEndInstance> endInstanceList;
    QVector<QmlRangeEventStartInstance> startInstanceList;

    QmlRangeEventData qmlRootEvent;

    QV8ProfilerDataModel *v8DataModel;

    QHash<int, QmlRangeEventTypeCount *> typeCounts;

    qint64 traceEndTime;
    qint64 traceStartTime;
    qint64 qmlMeasuredTime;

    int lastFrameEventIndex;
    qint64 maxAnimationCount;
    qint64 minAnimationCount;

    // file to load
    QString fileName;
};


////////////////////////////////////////////////////////////////////////////////////


QmlProfilerDataModel::QmlProfilerDataModel(QObject *parent) :
    QObject(parent), d(new QmlProfilerDataModelPrivate(this))
{
    setObjectName(QLatin1String("QmlProfilerDataModel"));

    d->listState = Empty;

    d->traceEndTime = 0;
    d->traceStartTime = -1;
    d->qmlMeasuredTime = 0;
    d->clearQmlRootEvent();
    d->lastFrameEventIndex = -1;
    d->maxAnimationCount = 0;
    d->minAnimationCount = 0;
    d->v8DataModel = new QV8ProfilerDataModel(this, this);
}

QmlProfilerDataModel::~QmlProfilerDataModel()
{
    clear();
    delete d;
}

////////////////////////////////////////////////////////////////////////////////////
QList<QmlRangeEventData *> QmlProfilerDataModel::getEventDescriptions() const
{
    return d->rangeEventDictionary.values();
}

QmlRangeEventData *QmlProfilerDataModel::eventDescription(int eventId) const
{
    foreach (QmlRangeEventData *event, d->rangeEventDictionary.values()) {
        if (event->eventId == eventId)
            return event;
    }
    return 0;
}

QList<QV8EventData *> QmlProfilerDataModel::getV8Events() const
{
    return d->v8DataModel->getV8Events();
}

QV8EventData *QmlProfilerDataModel::v8EventDescription(int eventId) const
{
    return d->v8DataModel->v8EventDescription(eventId);
}

////////////////////////////////////////////////////////////////////////////////////

void QmlProfilerDataModel::clear()
{
    qDeleteAll(d->rangeEventDictionary.values());
    d->rangeEventDictionary.clear();

    d->endInstanceList.clear();
    d->startInstanceList.clear();

    d->clearQmlRootEvent();

    foreach (QmlRangeEventTypeCount *typeCount, d->typeCounts.values())
        delete typeCount;
    d->typeCounts.clear();

    d->traceEndTime = 0;
    d->traceStartTime = -1;
    d->qmlMeasuredTime = 0;

    d->lastFrameEventIndex = -1;
    d->maxAnimationCount = 0;
    d->minAnimationCount = 0;

    d->v8DataModel->clear();

    emit countChanged();
    setState(Empty);
}

void QmlProfilerDataModel::prepareForWriting()
{
    setState(AcquiringData);
}

void QmlProfilerDataModel::addRangedEvent(int type, int bindingType, qint64 startTime,
                                          qint64 length, const QStringList &data,
                                          const QmlDebug::QmlEventLocation &location)
{
    const QChar colon = QLatin1Char(':');
    QString displayName, eventHashStr, details;
    QmlDebug::QmlEventLocation eventLocation = location;

    setState(AcquiringData);

    // generate details string
    if (data.isEmpty())
        details = tr("Source code not available");
    else {
        details = data.join(QLatin1String(" ")).replace(QLatin1Char('\n'),QLatin1Char(' ')).simplified();
        QRegExp rewrite(QLatin1String("\\(function \\$(\\w+)\\(\\) \\{ (return |)(.+) \\}\\)"));
        bool match = rewrite.exactMatch(details);
        if (match)
            details = rewrite.cap(1) + QLatin1String(": ") + rewrite.cap(3);
        if (details.startsWith(QLatin1String("file://")))
            details = details.mid(details.lastIndexOf(QLatin1Char('/')) + 1);
    }

    // backwards compatibility: "compiling" events don't have a proper location in older
    // version of the protocol, but the filename is passed in the details string
    if (type == QmlDebug::Compiling && eventLocation.filename.isEmpty()) {
        eventLocation.filename = details;
        eventLocation.line = 1;
        eventLocation.column = 1;
    }

    // generate hash
    if (eventLocation.filename.isEmpty()) {
        displayName = tr("<bytecode>");
        eventHashStr = getHashStringForQmlEvent(eventLocation, type);
    } else {
        const QString filePath = QUrl(eventLocation.filename).path();
        displayName = filePath.mid(filePath.lastIndexOf(QLatin1Char('/')) + 1) + colon +
                QString::number(eventLocation.line);
        eventHashStr = getHashStringForQmlEvent(eventLocation, type);
    }

    QmlRangeEventData *newEvent;
    if (d->rangeEventDictionary.contains(eventHashStr)) {
        newEvent = d->rangeEventDictionary[eventHashStr];
    } else {
        newEvent = new QmlRangeEventData;
        newEvent->displayName = displayName;
        newEvent->location = eventLocation;
        newEvent->eventHashStr = eventHashStr;
        newEvent->eventType = (QmlDebug::QmlEventType)type;
        newEvent->details = details;
        newEvent->bindingType = bindingType;
        d->rangeEventDictionary.insert(eventHashStr, newEvent);
    }

    QmlRangeEventEndInstance endTimeData;
    endTimeData.endTime = startTime + length;
    endTimeData.description = newEvent;
    endTimeData.startTimeIndex = d->startInstanceList.count();

    QmlRangeEventStartInstance startTimeData;
    startTimeData.startTime = startTime;
    startTimeData.duration = length;
    startTimeData.statsInfo = newEvent;
    startTimeData.endTimeIndex = d->endInstanceList.count();
    startTimeData.animationCount = -1;
    startTimeData.frameRate = 1e9/length;
    startTimeData.baseEventIndex = d->startInstanceList.count(); // point to itself by default

    d->endInstanceList << endTimeData;
    d->startInstanceList << startTimeData;

    emit countChanged();
}

void QmlProfilerDataModel::addV8Event(int depth, const QString &function,
                                      const QString &filename, int lineNumber,
                                      double totalTime, double selfTime)
{
    d->v8DataModel->addV8Event(depth, function, filename, lineNumber, totalTime, selfTime);
}

void QmlProfilerDataModel::addFrameEvent(qint64 time, int framerate, int animationcount)
{
    QString displayName, eventHashStr, details;

    setState(AcquiringData);

    details = tr("Animation Timer Update");
    displayName = tr("<Animation Update>");
    eventHashStr = displayName;

    QmlRangeEventData *newEvent;
    if (d->rangeEventDictionary.contains(eventHashStr)) {
        newEvent = d->rangeEventDictionary[eventHashStr];
    } else {
        newEvent = new QmlRangeEventData;
        newEvent->displayName = displayName;
        newEvent->eventHashStr = eventHashStr;
        newEvent->eventType = QmlDebug::Painting;
        newEvent->details = details;
        d->rangeEventDictionary.insert(eventHashStr, newEvent);
    }

    qint64 length = 1e9/framerate;
    // avoid overlap
    QmlRangeEventStartInstance *lastFrameEvent = 0;
    if (d->lastFrameEventIndex > -1) {
        lastFrameEvent = &d->startInstanceList[d->lastFrameEventIndex];
        if (lastFrameEvent->startTime + lastFrameEvent->duration >= time) {
            lastFrameEvent->duration = time - 1 - lastFrameEvent->startTime;
            d->endInstanceList[lastFrameEvent->endTimeIndex].endTime =
                    lastFrameEvent->startTime + lastFrameEvent->duration;
        }
    }

    QmlRangeEventEndInstance endTimeData;
    endTimeData.endTime = time + length;
    endTimeData.description = newEvent;
    endTimeData.startTimeIndex = d->startInstanceList.count();

    QmlRangeEventStartInstance startTimeData;
    startTimeData.startTime = time;
    startTimeData.duration = length;
    startTimeData.statsInfo = newEvent;
    startTimeData.endTimeIndex = d->endInstanceList.count();
    startTimeData.animationCount = animationcount;
    startTimeData.frameRate = framerate;
    startTimeData.baseEventIndex = d->startInstanceList.count(); // point to itself by default

    d->endInstanceList << endTimeData;
    d->startInstanceList << startTimeData;

    d->lastFrameEventIndex = d->startInstanceList.count() - 1;

    emit countChanged();
}

void QmlProfilerDataModel::setTraceEndTime(qint64 time)
{
    d->traceEndTime = time;
}

void QmlProfilerDataModel::setTraceStartTime(qint64 time)
{
    d->traceStartTime = time;
}

////////////////////////////////////////////////////////////////////////////////////

QString QmlProfilerDataModel::getHashStringForQmlEvent(
        const QmlDebug::QmlEventLocation &location, int eventType)
{
    return QString::fromLatin1("%1:%2:%3:%4").arg(location.filename,
                                      QString::number(location.line),
                                      QString::number(location.column),
                                      QString::number(eventType));
}

QString QmlProfilerDataModel::getHashStringForV8Event(const QString &displayName,
                                                      const QString &function)
{
    return QString::fromLatin1("%1:%2").arg(displayName, function);
}

QString QmlProfilerDataModel::rootEventName()
{
    return tr("<program>");
}

QString QmlProfilerDataModel::rootEventDescription()
{
    return tr("Main Program");
}

QString QmlProfilerDataModel::qmlEventTypeAsString(QmlEventType typeEnum)
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

QmlEventType QmlProfilerDataModel::qmlEventTypeAsEnum(const QString &typeString)
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
        if (isNumber)
            return (QmlEventType)type;
        else
            return MaximumQmlEventType;
    }
}

////////////////////////////////////////////////////////////////////////////////////

int QmlProfilerDataModel::findFirstIndex(qint64 startTime) const
{
    int candidate = -1;
    // in the "endtime" list, find the first event that ends after startTime
    if (d->endInstanceList.isEmpty())
        return 0; // -1
    if (d->endInstanceList.count() == 1 || d->endInstanceList.first().endTime >= startTime)
        candidate = 0;
    else
        if (d->endInstanceList.last().endTime <= startTime)
            return 0; // -1

    if (candidate == -1)
    {
        int fromIndex = 0;
        int toIndex = d->endInstanceList.count()-1;
        while (toIndex - fromIndex > 1) {
            int midIndex = (fromIndex + toIndex)/2;
            if (d->endInstanceList[midIndex].endTime < startTime)
                fromIndex = midIndex;
            else
                toIndex = midIndex;
        }

        candidate = toIndex;
    }

    int eventIndex = d->endInstanceList[candidate].startTimeIndex;
    return d->startInstanceList[eventIndex].baseEventIndex;
}

int QmlProfilerDataModel::findFirstIndexNoParents(qint64 startTime) const
{
    int candidate = -1;
    // in the "endtime" list, find the first event that ends after startTime
    if (d->endInstanceList.isEmpty())
        return 0; // -1
    if (d->endInstanceList.count() == 1 || d->endInstanceList.first().endTime >= startTime)
        candidate = 0;
    else
        if (d->endInstanceList.last().endTime <= startTime)
            return 0; // -1

    if (candidate == -1) {
        int fromIndex = 0;
        int toIndex = d->endInstanceList.count()-1;
        while (toIndex - fromIndex > 1) {
            int midIndex = (fromIndex + toIndex)/2;
            if (d->endInstanceList[midIndex].endTime < startTime)
                fromIndex = midIndex;
            else
                toIndex = midIndex;
        }

        candidate = toIndex;
    }

    int ndx = d->endInstanceList[candidate].startTimeIndex;

    return ndx;
}

int QmlProfilerDataModel::findLastIndex(qint64 endTime) const
{
    // in the "starttime" list, find the last event that starts before endtime
    if (d->startInstanceList.isEmpty())
        return 0; // -1
    if (d->startInstanceList.first().startTime >= endTime)
        return 0; // -1
    if (d->startInstanceList.count() == 1)
        return 0;
    if (d->startInstanceList.last().startTime <= endTime)
        return d->startInstanceList.count()-1;

    int fromIndex = 0;
    int toIndex = d->startInstanceList.count()-1;
    while (toIndex - fromIndex > 1) {
        int midIndex = (fromIndex + toIndex)/2;
        if (d->startInstanceList[midIndex].startTime < endTime)
            fromIndex = midIndex;
        else
            toIndex = midIndex;
    }

    return fromIndex;
}

qint64 QmlProfilerDataModel::firstTimeMark() const
{
    if (d->startInstanceList.isEmpty())
        return 0;
    else {
        return d->startInstanceList[0].startTime;
    }
}

qint64 QmlProfilerDataModel::lastTimeMark() const
{
    if (d->endInstanceList.isEmpty())
        return 0;
    else {
        return d->endInstanceList.last().endTime;
    }
}

////////////////////////////////////////////////////////////////////////////////////

int QmlProfilerDataModel::count() const
{
    return d->startInstanceList.count();
}

bool QmlProfilerDataModel::isEmpty() const
{
    return d->startInstanceList.isEmpty() && d->v8DataModel->isEmpty();
}

qint64 QmlProfilerDataModel::getStartTime(int index) const
{
    return d->startInstanceList[index].startTime;
}

qint64 QmlProfilerDataModel::getEndTime(int index) const
{
    return d->startInstanceList[index].startTime + d->startInstanceList[index].duration;
}

qint64 QmlProfilerDataModel::getDuration(int index) const
{
    return d->startInstanceList[index].duration;
}

int QmlProfilerDataModel::getType(int index) const
{
    return d->startInstanceList[index].statsInfo->eventType;
}

int QmlProfilerDataModel::getNestingLevel(int index) const
{
    return d->startInstanceList[index].nestingLevel;
}

int QmlProfilerDataModel::getNestingDepth(int index) const
{
    return d->startInstanceList[index].nestingDepth;
}

QString QmlProfilerDataModel::getFilename(int index) const
{
    return d->startInstanceList[index].statsInfo->location.filename;
}

int QmlProfilerDataModel::getLine(int index) const
{
    return d->startInstanceList[index].statsInfo->location.line;
}

int QmlProfilerDataModel::getColumn(int index) const
{
    return d->startInstanceList[index].statsInfo->location.column;
}

QString QmlProfilerDataModel::getDetails(int index) const
{
    // special: animations
    if (d->startInstanceList[index].statsInfo->eventType == QmlDebug::Painting &&
            d->startInstanceList[index].animationCount >= 0)
        return tr("%1 animations at %2 FPS").arg(
                    QString::number(d->startInstanceList[index].animationCount),
                    QString::number(d->startInstanceList[index].frameRate));
    return d->startInstanceList[index].statsInfo->details;
}

int QmlProfilerDataModel::getEventId(int index) const
{
    return d->startInstanceList[index].statsInfo->eventId;
}

int QmlProfilerDataModel::getBindingLoopDest(int index) const
{
    return d->startInstanceList[index].bindingLoopHead;
}

int QmlProfilerDataModel::getFramerate(int index) const
{
    return d->startInstanceList[index].frameRate;
}

int QmlProfilerDataModel::getAnimationCount(int index) const
{
    return d->startInstanceList[index].animationCount;
}

int QmlProfilerDataModel::getMaximumAnimationCount() const
{
    return d->maxAnimationCount;
}

int QmlProfilerDataModel::getMinimumAnimationCount() const
{
    return d->minAnimationCount;
}

/////////////////////////////////////////

int QmlProfilerDataModel::uniqueEventsOfType(int type) const
{
    if (!d->typeCounts.contains(type))
        return 0;
    return d->typeCounts[type]->eventIds.count();
}

int QmlProfilerDataModel::maxNestingForType(int type) const
{
    if (!d->typeCounts.contains(type))
        return 0;
    return d->typeCounts[type]->nestingCount;
}

QString QmlProfilerDataModel::eventTextForType(int type, int index) const
{
    if (!d->typeCounts.contains(type))
        return QString();
    return d->rangeEventDictionary.values().at(d->typeCounts[type]->eventIds[index])->details;
}

QString QmlProfilerDataModel::eventDisplayNameForType(int type, int index) const
{
    if (!d->typeCounts.contains(type))
        return QString();
    return d->rangeEventDictionary.values().at(d->typeCounts[type]->eventIds[index])->displayName;
}

int QmlProfilerDataModel::eventIdForType(int type, int index) const
{
    if (!d->typeCounts.contains(type))
        return -1;
    return d->typeCounts[type]->eventIds[index];
}

int QmlProfilerDataModel::eventPosInType(int index) const
{
    int eventType = d->startInstanceList[index].statsInfo->eventType;
    return d->typeCounts[eventType]->eventIds.indexOf(d->startInstanceList[index].statsInfo->eventId);
}

/////////////////////////////////////////

qint64 QmlProfilerDataModel::traceStartTime() const
{
    return d->traceStartTime != -1? d->traceStartTime : firstTimeMark();
}

qint64 QmlProfilerDataModel::traceEndTime() const
{
    return d->traceEndTime ? d->traceEndTime : lastTimeMark();
}

qint64 QmlProfilerDataModel::traceDuration() const
{
    return traceEndTime() - traceStartTime();
}

qint64 QmlProfilerDataModel::qmlMeasuredTime() const
{
    return d->qmlMeasuredTime;
}
qint64 QmlProfilerDataModel::v8MeasuredTime() const
{
    return d->v8DataModel->v8MeasuredTime();
}

////////////////////////////////////////////////////////////////////////////////////

void QmlProfilerDataModel::complete()
{
    if (currentState() == AcquiringData) {
        setState(ProcessingData);
        d->v8DataModel->collectV8Statistics();
        d->postProcess();
    } else
    if (currentState() == Empty) {
        d->v8DataModel->collectV8Statistics();
        compileStatistics(traceStartTime(), traceEndTime());
        setState(Done);
    } else {
        emit error(tr("Unexpected complete signal in data model"));
    }
}

void QmlProfilerDataModel::QmlProfilerDataModelPrivate::postProcess()
{
    if (q->count() != 0) {
        sortStartTimes();
        sortEndTimes();
        findAnimationLimits();
        computeNestingLevels();
        computeNestingDepth();
        linkEndsToStarts();
        insertQmlRootEvent();
        q->reloadDetails();
        prepareForDisplay();
        q->compileStatistics(q->traceStartTime(), q->traceEndTime());
    }
    q->setState(Done);
}


void QmlProfilerDataModel::QmlProfilerDataModelPrivate::prepareForDisplay()
{
    // generate numeric ids
    int ndx = 0;
    foreach (QmlRangeEventData *binding, rangeEventDictionary.values()) {
        binding->eventId = ndx++;
    }

    // collect type counts
    foreach (const QmlRangeEventStartInstance &eventStartData, startInstanceList) {
        int typeNumber = eventStartData.statsInfo->eventType;
        if (!typeCounts.contains(typeNumber)) {
            typeCounts[typeNumber] = new QmlRangeEventTypeCount;
            typeCounts[typeNumber]->nestingCount = 0;
        }
        if (eventStartData.nestingLevel > typeCounts[typeNumber]->nestingCount)
            typeCounts[typeNumber]->nestingCount = eventStartData.nestingLevel;
        if (!typeCounts[typeNumber]->eventIds.contains(eventStartData.statsInfo->eventId))
            typeCounts[typeNumber]->eventIds << eventStartData.statsInfo->eventId;
    }
}

void QmlProfilerDataModel::QmlProfilerDataModelPrivate::sortStartTimes()
{
    if (startInstanceList.count() < 2)
        return;

    // assuming startTimes is partially sorted
    // identify blocks of events and sort them with quicksort
    QVector<QmlRangeEventStartInstance>::iterator itFrom = startInstanceList.end() - 2;
    QVector<QmlRangeEventStartInstance>::iterator itTo = startInstanceList.end() - 1;

    while (itFrom != startInstanceList.begin() && itTo != startInstanceList.begin()) {
        // find block to sort
        while (itFrom != startInstanceList.begin()
                && itTo->startTime > itFrom->startTime) {
            itTo--;
            itFrom = itTo - 1;
        }

        // if we're at the end of the list
        if (itFrom == startInstanceList.begin())
            break;

        // find block length
        while (itFrom != startInstanceList.begin()
                && itTo->startTime <= itFrom->startTime)
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
    linkEndsToStarts();
}

void QmlProfilerDataModel::QmlProfilerDataModelPrivate::sortEndTimes()
{
    // assuming endTimes is partially sorted
    // identify blocks of events and sort them with quicksort

    if (endInstanceList.count() < 2)
        return;

    QVector<QmlRangeEventEndInstance>::iterator itFrom = endInstanceList.begin();
    QVector<QmlRangeEventEndInstance>::iterator itTo = endInstanceList.begin() + 1;

    while (itTo != endInstanceList.end() && itFrom != endInstanceList.end()) {
        // find block to sort
        while (itTo != endInstanceList.end()
                && startInstanceList[itTo->startTimeIndex].startTime >
                startInstanceList[itFrom->startTimeIndex].startTime +
                startInstanceList[itFrom->startTimeIndex].duration) {
            itFrom++;
            itTo = itFrom+1;
        }

        // if we're at the end of the list
        if (itTo == endInstanceList.end())
            break;

        // find block length
        while (itTo != endInstanceList.end()
                && startInstanceList[itTo->startTimeIndex].startTime <=
                startInstanceList[itFrom->startTimeIndex].startTime +
                startInstanceList[itFrom->startTimeIndex].duration)
            itTo++;

        // sort block
        qSort(itFrom, itTo, compareEndTimes);

        // move to next block
        itFrom = itTo;
        itTo = itFrom+1;

    }

    // link back the startTimes
    linkStartsToEnds();
}

void QmlProfilerDataModel::QmlProfilerDataModelPrivate::linkStartsToEnds()
{
    for (int i = 0; i < endInstanceList.count(); i++)
        startInstanceList[endInstanceList[i].startTimeIndex].endTimeIndex = i;
}

void QmlProfilerDataModel::QmlProfilerDataModelPrivate::findAnimationLimits()
{
    maxAnimationCount = 0;
    minAnimationCount = 0;
    lastFrameEventIndex = -1;

    for (int i = 0; i < startInstanceList.count(); i++) {
        if (startInstanceList[i].statsInfo->eventType == QmlDebug::Painting &&
                startInstanceList[i].animationCount >= 0) {
            int animationcount = startInstanceList[i].animationCount;
            if (lastFrameEventIndex > -1) {
                if (animationcount > maxAnimationCount)
                    maxAnimationCount = animationcount;
                if (animationcount < minAnimationCount)
                    minAnimationCount = animationcount;
            } else {
                maxAnimationCount = animationcount;
                minAnimationCount = animationcount;
            }
            lastFrameEventIndex = i;
        }
    }
}

void QmlProfilerDataModel::QmlProfilerDataModelPrivate::computeNestingLevels()
{
    // compute levels
    QHash<int, qint64> endtimesPerLevel;
    QList<int> nestingLevels;
    QList< QHash<int, qint64> > endtimesPerNestingLevel;
    int level = Constants::QML_MIN_LEVEL;
    endtimesPerLevel[Constants::QML_MIN_LEVEL] = 0;
    int lastBaseEventIndex = 0;
    qint64 lastBaseEventEndTime = traceStartTime;

    for (int i = 0; i < QmlDebug::MaximumQmlEventType; i++) {
        nestingLevels << Constants::QML_MIN_LEVEL;
        QHash<int, qint64> dummyHash;
        dummyHash[Constants::QML_MIN_LEVEL] = 0;
        endtimesPerNestingLevel << dummyHash;
    }

    for (int i=0; i<startInstanceList.count(); i++) {
        qint64 st = startInstanceList[i].startTime;
        int type = startInstanceList[i].statsInfo->eventType;

        if (type == QmlDebug::Painting) {
            // animation/paint events have level 0 by definition (same as "mainprogram"),
            // but are not considered parents of other events for statistical purposes
            startInstanceList[i].level = Constants::QML_MIN_LEVEL - 1;
            startInstanceList[i].nestingLevel = Constants::QML_MIN_LEVEL;
            if (lastBaseEventEndTime < startInstanceList[i].startTime) {
                lastBaseEventIndex = i;
                lastBaseEventEndTime = startInstanceList[i].startTime + startInstanceList[i].duration;
            }
            startInstanceList[i].baseEventIndex = lastBaseEventIndex;
            continue;
        }

        // general level
        if (endtimesPerLevel[level] > st) {
            level++;
        } else {
            while (level > Constants::QML_MIN_LEVEL && endtimesPerLevel[level-1] <= st)
                level--;
        }
        endtimesPerLevel[level] = st + startInstanceList[i].duration;

        // per type
        if (endtimesPerNestingLevel[type][nestingLevels[type]] > st) {
            nestingLevels[type]++;
        } else {
            while (nestingLevels[type] > Constants::QML_MIN_LEVEL &&
                   endtimesPerNestingLevel[type][nestingLevels[type]-1] <= st)
                nestingLevels[type]--;
        }
        endtimesPerNestingLevel[type][nestingLevels[type]] =
                st + startInstanceList[i].duration;

        startInstanceList[i].level = level;
        startInstanceList[i].nestingLevel = nestingLevels[type];

        if (level == Constants::QML_MIN_LEVEL) {
            qmlMeasuredTime += startInstanceList[i].duration;
            if (lastBaseEventEndTime < startInstanceList[i].startTime) {
                lastBaseEventIndex = i;
                lastBaseEventEndTime = startInstanceList[i].startTime + startInstanceList[i].duration;
            }
        }
        startInstanceList[i].baseEventIndex = lastBaseEventIndex;
    }
}

void QmlProfilerDataModel::QmlProfilerDataModelPrivate::computeNestingDepth()
{
    QHash<int, int> nestingDepth;
    for (int i = 0; i < endInstanceList.count(); i++) {
        int type = endInstanceList[i].description->eventType;
        int nestingInType = startInstanceList[endInstanceList[i].startTimeIndex].nestingLevel;
        if (!nestingDepth.contains(type))
            nestingDepth[type] = nestingInType;
        else {
            int nd = nestingDepth[type];
            nestingDepth[type] = nd > nestingInType ? nd : nestingInType;
        }

        startInstanceList[endInstanceList[i].startTimeIndex].nestingDepth = nestingDepth[type];
        if (nestingInType == Constants::QML_MIN_LEVEL)
            nestingDepth[type] = Constants::QML_MIN_LEVEL;
    }
}

void QmlProfilerDataModel::QmlProfilerDataModelPrivate::linkEndsToStarts()
{
    for (int i = 0; i < startInstanceList.count(); i++)
        endInstanceList[startInstanceList[i].endTimeIndex].startTimeIndex = i;
}

////////////////////////////////////////////////////////////////////////////////////

void QmlProfilerDataModel::compileStatistics(qint64 startTime, qint64 endTime)
{
    d->clearStatistics();
    if (traceDuration() > 0) {
        if (count() > 0) {
            d->redoTree(startTime, endTime);
            d->computeMedianTime(startTime, endTime);
            d->findBindingLoops(startTime, endTime);
        } else {
            d->insertQmlRootEvent();
            QmlRangeEventData *listedRootEvent = d->rangeEventDictionary.value(rootEventName());
            listedRootEvent->calls = 1;
            listedRootEvent->percentOfTime = 100;
        }
    }
}

void QmlProfilerDataModel::QmlProfilerDataModelPrivate::clearStatistics()
{
    // clear existing statistics
    foreach (QmlRangeEventData *eventDescription, rangeEventDictionary.values()) {
        eventDescription->calls = 0;
        // maximum possible value
        eventDescription->minTime = traceEndTime;
        eventDescription->maxTime = 0;
        eventDescription->medianTime = 0;
        eventDescription->duration = 0;
        qDeleteAll(eventDescription->parentHash);
        qDeleteAll(eventDescription->childrenHash);
        eventDescription->parentHash.clear();
        eventDescription->childrenHash.clear();
    }
}

void QmlProfilerDataModel::QmlProfilerDataModelPrivate::redoTree(qint64 fromTime,
                                                                 qint64 toTime)
{
    double totalTime = 0;
    int fromIndex = q->findFirstIndex(fromTime);
    int toIndex = q->findLastIndex(toTime);

    QmlRangeEventData *listedRootEvent = rangeEventDictionary.value(rootEventName());
    QTC_ASSERT(listedRootEvent, /**/);

    // compute parent-child relationship and call count
    QHash<int, QmlRangeEventData*> lastParent;
    for (int index = fromIndex; index <= toIndex; index++) {
        QmlRangeEventData *eventDescription = startInstanceList[index].statsInfo;

        if (startInstanceList[index].startTime > toTime ||
                startInstanceList[index].startTime+startInstanceList[index].duration < fromTime) {
            continue;
        }

        if (eventDescription->eventType == QmlDebug::Painting) {
            // skip animation/paint events
            continue;
        }

        eventDescription->calls++;
        qint64 duration = startInstanceList[index].duration;
        eventDescription->duration += duration;
        if (eventDescription->maxTime < duration)
            eventDescription->maxTime = duration;
        if (eventDescription->minTime > duration)
            eventDescription->minTime = duration;

        int level = startInstanceList[index].level;

        QmlRangeEventData *parentEvent = listedRootEvent;
        if (level > Constants::QML_MIN_LEVEL && lastParent.contains(level-1))
            parentEvent = lastParent[level-1];

        if (!eventDescription->parentHash.contains(parentEvent->eventHashStr)) {
            QmlRangeEventRelative *newParentEvent = new QmlRangeEventRelative(parentEvent);
            newParentEvent->calls = 1;
            newParentEvent->duration = duration;

            eventDescription->parentHash.insert(parentEvent->eventHashStr, newParentEvent);
        } else {
            QmlRangeEventRelative *newParentEvent =
                    eventDescription->parentHash.value(parentEvent->eventHashStr);
            newParentEvent->duration += duration;
            newParentEvent->calls++;
        }

        if (!parentEvent->childrenHash.contains(eventDescription->eventHashStr)) {
            QmlRangeEventRelative *newChildEvent = new QmlRangeEventRelative(eventDescription);
            newChildEvent->calls = 1;
            newChildEvent->duration = duration;

            parentEvent->childrenHash.insert(eventDescription->eventHashStr, newChildEvent);
        } else {
            QmlRangeEventRelative *newChildEvent =
                    parentEvent->childrenHash.value(eventDescription->eventHashStr);
            newChildEvent->duration += duration;
            newChildEvent->calls++;
        }

        lastParent[level] = eventDescription;

        if (level == Constants::QML_MIN_LEVEL)
            totalTime += duration;
    }

    // fake rootEvent statistics
    // the +1 nanosecond is to force it to be on top of the sorted list
    listedRootEvent->duration = totalTime+1;
    listedRootEvent->minTime = totalTime+1;
    listedRootEvent->maxTime = totalTime+1;
    listedRootEvent->medianTime = totalTime+1;
    if (totalTime > 0)
        listedRootEvent->calls = 1;

    // copy to the global root reference
    qmlRootEvent = *listedRootEvent;

    // compute percentages
    foreach (QmlRangeEventData *binding, rangeEventDictionary.values()) {
        binding->percentOfTime = binding->duration * 100.0 / totalTime;
        binding->timePerCall = binding->calls > 0 ? double(binding->duration) / binding->calls : 0;
    }
}

void QmlProfilerDataModel::QmlProfilerDataModelPrivate::computeMedianTime(qint64 fromTime,
                                                                          qint64 toTime)
{
    int fromIndex = q->findFirstIndex(fromTime);
    int toIndex = q->findLastIndex(toTime);

    // compute median time
    QHash< QmlRangeEventData* , QList<qint64> > durationLists;
    for (int index = fromIndex; index <= toIndex; index++) {
        QmlRangeEventData *desc = startInstanceList[index].statsInfo;
        qint64 len = startInstanceList[index].duration;
        durationLists[desc].append(len);
    }
    QMutableHashIterator < QmlRangeEventData* , QList<qint64> > iter(durationLists);
    while (iter.hasNext()) {
        iter.next();
        if (!iter.value().isEmpty()) {
            qSort(iter.value());
            iter.key()->medianTime = iter.value().at(iter.value().count()/2);
        }
    }
}

void QmlProfilerDataModel::QmlProfilerDataModelPrivate::findBindingLoops(qint64 fromTime,
                                                                         qint64 toTime)
{
    int fromIndex = q->findFirstIndex(fromTime);
    int toIndex = q->findLastIndex(toTime);

    // first clear existing data
    foreach (QmlRangeEventData *event, rangeEventDictionary.values()) {
        event->isBindingLoop = false;
        foreach (QmlRangeEventRelative *parentEvent, event->parentHash.values())
            parentEvent->inLoopPath = false;
        foreach (QmlRangeEventRelative *childEvent, event->childrenHash.values())
            childEvent->inLoopPath = false;
    }

    QList<QmlRangeEventData *> stackRefs;
    QList<QmlRangeEventStartInstance *> stack;

    for (int i = 0; i < startInstanceList.count(); i++) {
        QmlRangeEventData *currentEvent = startInstanceList[i].statsInfo;
        QmlRangeEventStartInstance *inTimeEvent = &startInstanceList[i];
        inTimeEvent->bindingLoopHead = -1;

        // special: skip animation events (but not old paint events)
        if (inTimeEvent->statsInfo->eventType == Painting && inTimeEvent->animationCount >= 0)
            continue;

        // managing call stack
        for (int j = stack.count() - 1; j >= 0; j--) {
            if (stack[j]->startTime + stack[j]->duration <= inTimeEvent->startTime) {
                stack.removeAt(j);
                stackRefs.removeAt(j);
            }
        }

        bool loopDetected = stackRefs.contains(currentEvent);
        stack << inTimeEvent;
        stackRefs << currentEvent;

        if (loopDetected) {
            if (i >= fromIndex && i <= toIndex) {
                // for the statistics
                currentEvent->isBindingLoop = true;
                for (int j = stackRefs.indexOf(currentEvent); j < stackRefs.count()-1; j++) {
                    QmlRangeEventRelative *nextEventSub =
                            stackRefs[j]->childrenHash.value(stackRefs[j+1]->eventHashStr);
                    nextEventSub->inLoopPath = true;
                    QmlRangeEventRelative *prevEventSub =
                            stackRefs[j+1]->parentHash.value(stackRefs[j]->eventHashStr);
                    prevEventSub->inLoopPath = true;
                }
            }

            // use crossed references to find index in starttimesortedlist
            QmlRangeEventStartInstance *head = stack[stackRefs.indexOf(currentEvent)];
            inTimeEvent->bindingLoopHead = endInstanceList[head->endTimeIndex].startTimeIndex;
            startInstanceList[inTimeEvent->bindingLoopHead].bindingLoopHead = i;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////

void QmlProfilerDataModel::QmlProfilerDataModelPrivate::clearQmlRootEvent()
{
    qmlRootEvent.displayName = rootEventName();
    qmlRootEvent.location = QmlEventLocation();
    qmlRootEvent.eventHashStr = rootEventName();
    qmlRootEvent.details = rootEventDescription();
    qmlRootEvent.eventType = QmlDebug::Binding;
    qmlRootEvent.duration = 0;
    qmlRootEvent.calls = 0;
    qmlRootEvent.minTime = 0;
    qmlRootEvent.maxTime = 0;
    qmlRootEvent.timePerCall = 0;
    qmlRootEvent.percentOfTime = 0;
    qmlRootEvent.medianTime = 0;
    qmlRootEvent.eventId = -1;

    qDeleteAll(qmlRootEvent.parentHash.values());
    qDeleteAll(qmlRootEvent.childrenHash.values());
    qmlRootEvent.parentHash.clear();
    qmlRootEvent.childrenHash.clear();
}

void QmlProfilerDataModel::QmlProfilerDataModelPrivate::insertQmlRootEvent()
{
    // create root event for statistics & insert into list
    clearQmlRootEvent();
    QmlRangeEventData *listedRootEvent = rangeEventDictionary.value(rootEventName());
    if (!listedRootEvent) {
        listedRootEvent = new QmlRangeEventData;
        rangeEventDictionary.insert(rootEventName(), listedRootEvent);
    }
    *listedRootEvent = qmlRootEvent;
}

void QmlProfilerDataModel::reloadDetails()
{
    // request binding/signal details from the AST
    foreach (QmlRangeEventData *event, d->rangeEventDictionary.values()) {
        if (event->eventType != Binding && event->eventType != HandlingSignal)
            continue;

        // This skips anonymous bindings in Qt4.8 (we don't have valid location data for them)
        if (event->location.filename.isEmpty())
            continue;

        // Skip non-anonymous bindings from Qt4.8 (we already have correct details for them)
        if (event->location.column == -1)
            continue;

        emit requestDetailsForLocation(event->eventType, event->location);
    }
    emit reloadDocumentsForDetails();
}

void QmlProfilerDataModel::rewriteDetailsString(int eventType,
                                                const QmlDebug::QmlEventLocation &location,
                                                const QString &newString)
{
    QString eventHashStr = getHashStringForQmlEvent(location, eventType);
    QTC_ASSERT(d->rangeEventDictionary.contains(eventHashStr), return);
    d->rangeEventDictionary.value(eventHashStr)->details = newString;
    emit detailsChanged(d->rangeEventDictionary.value(eventHashStr)->eventId, newString);
}

void QmlProfilerDataModel::finishedRewritingDetails()
{
    emit reloadDetailLabels();
}

////////////////////////////////////////////////////////////////////////////////////

bool QmlProfilerDataModel::save(const QString &filename)
{
    if (isEmpty()) {
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

    stream.writeStartElement(QLatin1String("trace"));
    stream.writeAttribute(QLatin1String("version"), QLatin1String(Constants::PROFILER_FILE_VERSION));

    stream.writeAttribute(QLatin1String("traceStart"), QString::number(traceStartTime()));
    stream.writeAttribute(QLatin1String("traceEnd"), QString::number(traceEndTime()));

    stream.writeStartElement(QLatin1String("eventData"));
    stream.writeAttribute(QLatin1String("totalTime"), QString::number(d->qmlMeasuredTime));

    foreach (const QmlRangeEventData *eventData, d->rangeEventDictionary.values()) {
        stream.writeStartElement(QLatin1String("event"));
        stream.writeAttribute(QLatin1String("index"), QString::number(d->rangeEventDictionary.keys().indexOf(eventData->eventHashStr)));
        stream.writeTextElement(QLatin1String("displayname"), eventData->displayName);
        stream.writeTextElement(QLatin1String("type"), qmlEventTypeAsString(eventData->eventType));
        if (!eventData->location.filename.isEmpty()) {
            stream.writeTextElement(QLatin1String("filename"), eventData->location.filename);
            stream.writeTextElement(QLatin1String("line"), QString::number(eventData->location.line));
            stream.writeTextElement(QLatin1String("column"), QString::number(eventData->location.column));
        }
        stream.writeTextElement(QLatin1String("details"), eventData->details);
        if (eventData->eventType == Binding)
            stream.writeTextElement(QLatin1String("bindingType"), QString::number((int)eventData->bindingType));
        stream.writeEndElement();
    }
    stream.writeEndElement(); // eventData

    stream.writeStartElement(QLatin1String("profilerDataModel"));
    foreach (const QmlRangeEventStartInstance &rangedEvent, d->startInstanceList) {
        stream.writeStartElement(QLatin1String("range"));
        stream.writeAttribute(QLatin1String("startTime"), QString::number(rangedEvent.startTime));
        stream.writeAttribute(QLatin1String("duration"), QString::number(rangedEvent.duration));
        stream.writeAttribute(QLatin1String("eventIndex"), QString::number(d->rangeEventDictionary.keys().indexOf(rangedEvent.statsInfo->eventHashStr)));
        if (rangedEvent.statsInfo->eventType == QmlDebug::Painting && rangedEvent.animationCount >= 0) {
            // animation frame
            stream.writeAttribute(QLatin1String("framerate"), QString::number(rangedEvent.frameRate));
            stream.writeAttribute(QLatin1String("animationcount"), QString::number(rangedEvent.animationCount));
        }
        stream.writeEndElement();
    }
    stream.writeEndElement(); // profilerDataModel

    d->v8DataModel->save(stream);

    stream.writeEndElement(); // trace
    stream.writeEndDocument();

    file.close();
    return true;
}

void QmlProfilerDataModel::setFilename(const QString &filename)
{
    d->fileName = filename;
}

void QmlProfilerDataModel::load(const QString &filename)
{
    setFilename(filename);
    load();
}

// "be strict in your output but tolerant in your inputs"
void QmlProfilerDataModel::load()
{
    QString filename = d->fileName;

    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit error(tr("Could not open %1 for reading").arg(filename));
        return;
    }

    // erase current
    clear();

    setState(AcquiringData);

    bool readingQmlEvents = false;
    QHash<int, QmlRangeEventData *> descriptionBuffer;
    QmlRangeEventData *currentEvent = 0;
    bool startTimesAreSorted = true;
    bool validVersion = true;

    // time computation
    d->qmlMeasuredTime = 0;

    QXmlStreamReader stream(&file);

    while (validVersion && !stream.atEnd() && !stream.hasError()) {
        QXmlStreamReader::TokenType token = stream.readNext();
        QString elementName = stream.name().toString();
        switch (token) {
        case QXmlStreamReader::StartDocument :  continue;
        case QXmlStreamReader::StartElement : {
            if (elementName == QLatin1String("trace")) {
                QXmlStreamAttributes attributes = stream.attributes();
                if (attributes.hasAttribute(QLatin1String("version")))
                    validVersion = attributes.value(QLatin1String("version")).toString() == QLatin1String(Constants::PROFILER_FILE_VERSION);
                else
                    validVersion = false;
                if (attributes.hasAttribute(QLatin1String("traceStart")))
                    setTraceStartTime(attributes.value(QLatin1String("traceStart")).toString().toLongLong());
                if (attributes.hasAttribute(QLatin1String("traceEnd")))
                    setTraceEndTime(attributes.value(QLatin1String("traceEnd")).toString().toLongLong());
            }
            if (elementName == QLatin1String("eventData")) {
                readingQmlEvents = true;
                QXmlStreamAttributes attributes = stream.attributes();
                if (attributes.hasAttribute(QLatin1String("totalTime")))
                    d->qmlMeasuredTime = attributes.value(QLatin1String("totalTime")).toString().toDouble();
                break;
            }
            if (elementName == QLatin1String("v8profile") && !readingQmlEvents) {
                d->v8DataModel->load(stream);
                break;
            }

            if (elementName == QLatin1String("trace")) {
                QXmlStreamAttributes attributes = stream.attributes();
                if (attributes.hasAttribute(QLatin1String("traceStart")))
                    setTraceStartTime(attributes.value(QLatin1String("traceStart")).toString().toLongLong());
                if (attributes.hasAttribute(QLatin1String("traceEnd")))
                    setTraceEndTime(attributes.value(QLatin1String("traceEnd")).toString().toLongLong());
            }

            if (elementName == QLatin1String("range")) {
                QmlRangeEventStartInstance rangedEvent;
                QXmlStreamAttributes attributes = stream.attributes();
                if (attributes.hasAttribute(QLatin1String("startTime")))
                    rangedEvent.startTime = attributes.value(QLatin1String("startTime")).toString().toLongLong();
                if (attributes.hasAttribute(QLatin1String("duration")))
                    rangedEvent.duration = attributes.value(QLatin1String("duration")).toString().toLongLong();
                if (attributes.hasAttribute(QLatin1String("framerate")))
                    rangedEvent.frameRate = attributes.value(QLatin1String("framerate")).toString().toInt();
                if (attributes.hasAttribute(QLatin1String("animationcount")))
                    rangedEvent.animationCount = attributes.value(QLatin1String("animationcount")).toString().toInt();
                else
                    rangedEvent.animationCount = -1;
                if (attributes.hasAttribute(QLatin1String("eventIndex"))) {
                    int ndx = attributes.value(QLatin1String("eventIndex")).toString().toInt();
                    if (!descriptionBuffer.value(ndx))
                        descriptionBuffer[ndx] = new QmlRangeEventData;
                    rangedEvent.statsInfo = descriptionBuffer.value(ndx);
                }
                rangedEvent.endTimeIndex = d->endInstanceList.count();

                if (!d->startInstanceList.isEmpty()
                        && rangedEvent.startTime < d->startInstanceList.last().startTime)
                    startTimesAreSorted = false;
                d->startInstanceList << rangedEvent;

                QmlRangeEventEndInstance endTimeEvent;
                endTimeEvent.endTime = rangedEvent.startTime + rangedEvent.duration;
                endTimeEvent.startTimeIndex = d->startInstanceList.count()-1;
                endTimeEvent.description = rangedEvent.statsInfo;
                d->endInstanceList << endTimeEvent;
                break;
            }

            if (readingQmlEvents) {
                if (elementName == QLatin1String("event")) {
                    QXmlStreamAttributes attributes = stream.attributes();
                    if (attributes.hasAttribute(QLatin1String("index"))) {
                        int ndx = attributes.value(QLatin1String("index")).toString().toInt();
                        if (!descriptionBuffer.value(ndx))
                            descriptionBuffer[ndx] = new QmlRangeEventData;
                        currentEvent = descriptionBuffer[ndx];
                        // backwards compatibility: default bindingType
                        currentEvent->bindingType = QmlBinding;
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

                if (elementName == QLatin1String("displayname")) {
                    currentEvent->displayName = readData;
                    break;
                }
                if (elementName == QLatin1String("type")) {
                    currentEvent->eventType = qmlEventTypeAsEnum(readData);
                    break;
                }
                if (elementName == QLatin1String("filename")) {
                    currentEvent->location.filename = readData;
                    break;
                }
                if (elementName == QLatin1String("line")) {
                    currentEvent->location.line = readData.toInt();
                    break;
                }
                if (elementName == QLatin1String("column"))
                    currentEvent->location.column = readData.toInt();
                if (elementName == QLatin1String("details")) {
                    currentEvent->details = readData;
                    break;
                }
                if (elementName == QLatin1String("bindingType")) {
                    currentEvent->bindingType = readData.toInt();
                    break;
                }
            }
            break;
        }
        case QXmlStreamReader::EndElement : {
            if (elementName == QLatin1String("event")) {
                currentEvent = 0;
                break;
            }
            if (elementName == QLatin1String("eventData")) {
                readingQmlEvents = false;
                break;
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
        emit error(tr("Invalid version of QML Trace file."));
        return;
    }

    // move the buffered data to the details cache
    foreach (QmlRangeEventData *desc, descriptionBuffer.values()) {
        desc->eventHashStr = getHashStringForQmlEvent(desc->location, desc->eventType);;
        d->rangeEventDictionary[desc->eventHashStr] = desc;
    }

    // sort startTimeSortedList
    if (!startTimesAreSorted) {
        qSort(d->startInstanceList.begin(), d->startInstanceList.end(), compareStartTimes);
        for (int i = 0; i< d->startInstanceList.count(); i++) {
            QmlRangeEventStartInstance startTimeData = d->startInstanceList[i];
            d->endInstanceList[startTimeData.endTimeIndex].startTimeIndex = i;
        }
        qSort(d->endInstanceList.begin(), d->endInstanceList.end(), compareStartIndexes);
    }

    emit countChanged();

    descriptionBuffer.clear();

    setState(ProcessingData);
    d->postProcess();
}

////////////////////////////////////////////////////////////////////////////////////

QmlProfilerDataModel::State QmlProfilerDataModel::currentState() const
{
    return d->listState;
}

int QmlProfilerDataModel::getCurrentStateFromQml() const
{
    return (int)d->listState;
}

void QmlProfilerDataModel::setState(QmlProfilerDataModel::State state)
{
    // It's not an error, we are continuously calling "AcquiringData" for example
    if (d->listState == state)
        return;

    switch (state) {
        case Empty:
            // if it's not empty, complain but go on
            QTC_ASSERT(isEmpty(), /**/);
        break;
        case AcquiringData:
            // we're not supposed to receive new data while processing older data
            QTC_ASSERT(d->listState != ProcessingData, return);
        break;
        case ProcessingData:
            QTC_ASSERT(d->listState == AcquiringData, return);
        break;
        case Done:
            QTC_ASSERT(d->listState == ProcessingData || d->listState == Empty, return);
        break;
        default:
            emit error(tr("Trying to set unknown state in events list"));
        break;
    }

    d->listState = state;
    emit stateChanged();

    return;
}

} // namespace Internal
} // namespace QmlProfiler

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(QmlProfiler::Internal::QmlRangeEventData, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(QmlProfiler::Internal::QmlRangeEventStartInstance, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(QmlProfiler::Internal::QmlRangeEventEndInstance, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(QmlProfiler::Internal::QmlRangeEventRelative, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
