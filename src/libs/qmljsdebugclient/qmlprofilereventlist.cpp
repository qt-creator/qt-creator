/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
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
}

#define MIN_LEVEL 1

// description
struct QmlEventDescription {
    QmlEventDescription() : displayname(0), location(0), filename(0), details(0) {}
    ~QmlEventDescription() {
        delete displayname;
        delete location;
        delete filename;
        delete details;
    }

    QString *displayname;
    QString *location;
    QString *filename;
    QString *details;
    int eventType;
    int line;
};

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

    // Stored data
    QmlEventHash m_eventDescriptions;
    QList<QmlEventEndTimeData> m_endTimeSortedList;
    QList<QmlEventStartTimeData> m_startTimeSortedList;

    // file to load
    QString m_filename;
    ParsingStatus m_parsingStatus;
};


////////////////////////////////////////////////////////////////////////////////////


QmlProfilerEventList::QmlProfilerEventList(QObject *parent) :
    QObject(parent), d(new QmlProfilerEventListPrivate(this))
{
    d->m_parsingStatus = DoneStatus;
    setObjectName("QmlProfilerEventStatistics");
}

QmlProfilerEventList::~QmlProfilerEventList()
{
    clear();
}

void QmlProfilerEventList::clear()
{
    foreach (QmlEventData *binding, d->m_eventDescriptions.values())
        delete binding;
    d->m_eventDescriptions.clear();

    d->m_endTimeSortedList.clear();
    d->m_startTimeSortedList.clear();
    emit countChanged();
}

QList <QmlEventData *> QmlProfilerEventList::getEventDescriptions() const
{
    return d->m_eventDescriptions.values();
}

void QmlProfilerEventList::addRangedEvent(int type, qint64 startTime, qint64 length,
                                                const QStringList &data, const QString &fileName, int line)
{
    setParsingStatus(GettingDataStatus);

    const QChar colon = QLatin1Char(':');
    QString displayName, location, details;

    if (fileName.isEmpty()) {
        displayName = tr("<bytecode>");
        location = QString("--:%1:%2").arg(QString::number(type), data.join(" "));
    } else {
        const QString filePath = QUrl(fileName).path();
        displayName = filePath.mid(filePath.lastIndexOf(QChar('/')) + 1) + colon + QString::number(line);
        location = fileName+colon+QString::number(line);
    }

    QmlEventData *newEvent;
    if (d->m_eventDescriptions.contains(location)) {
        newEvent = d->m_eventDescriptions[location];
    } else {

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

        newEvent = new QmlEventData;
        newEvent->displayname = displayName;
        newEvent->filename = fileName;
        newEvent->location = location;
        newEvent->line = line;
        newEvent->eventType = (QmlJsDebugClient::QmlEventType)type;
        newEvent->details = details;
        d->m_eventDescriptions.insert(location, newEvent);
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

void QmlProfilerEventList::complete()
{
    postProcess();
}

void QmlProfilerEventList::compileStatistics()
{
    // clear existing statistics
    foreach (QmlEventData *eventDescription, d->m_eventDescriptions.values()) {
        eventDescription->calls = 0;
        // maximum possible value
        eventDescription->minTime = d->m_endTimeSortedList.last().endTime;
        eventDescription->maxTime = 0;
        eventDescription->medianTime = 0;
        eventDescription->cumulatedDuration = 0;
        eventDescription->parentList.clear();
        eventDescription->childrenList.clear();
    }

    // compute parent-child relationship and call count
    QHash<int, QmlEventData*> lastParent;
    foreach (QmlEventStartTimeData eventStartData, d->m_startTimeSortedList) {
        QmlEventData *eventDescription = eventStartData.description;
        eventDescription->calls++;
        eventDescription->cumulatedDuration += eventStartData.length;
        if (eventDescription->maxTime < eventStartData.length)
            eventDescription->maxTime = eventStartData.length;
        if (eventDescription->minTime > eventStartData.length)
            eventDescription->minTime = eventStartData.length;


        if (eventStartData.level > MIN_LEVEL) {
            if (lastParent.contains(eventStartData.level-1)) {
                QmlEventData *parentEvent = lastParent[eventStartData.level-1];
                if (!eventDescription->parentList.contains(parentEvent))
                    eventDescription->parentList.append(parentEvent);
                if (!parentEvent->childrenList.contains(eventDescription))
                    parentEvent->childrenList.append(eventDescription);
            }
        }

        lastParent[eventStartData.level] = eventDescription;
    }

    // compute percentages
    double totalTime = 0;
    foreach (QmlEventData *binding, d->m_eventDescriptions.values()) {
        if (binding->filename.isEmpty())
            continue;
        totalTime += binding->cumulatedDuration;
    }

    foreach (QmlEventData *binding, d->m_eventDescriptions.values()) {
        if (binding->filename.isEmpty())
            continue;
        binding->percentOfTime = binding->cumulatedDuration * 100.0 / totalTime;
        binding->timePerCall = binding->calls > 0 ? double(binding->cumulatedDuration) / binding->calls : 0;
    }

    // compute median time
    QHash < QmlEventData* , QList<qint64> > durationLists;
    foreach (const QmlEventStartTimeData &startData, d->m_startTimeSortedList) {
        durationLists[startData.description].append(startData.length);
    }
    QMutableHashIterator < QmlEventData* , QList<qint64> > iter(durationLists);
    while (iter.hasNext()) {
        iter.next();
        if (!iter.value().isEmpty()) {
            qSort(iter.value());
            iter.key()->medianTime = iter.value().at(iter.value().count()/2);
        }
    }

    // continue postprocess
    postProcess();
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

    // continue postprocess
    postProcess();
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

    // continue postprocess
    postProcess();
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
    switch (d->m_parsingStatus) {
    case GettingDataStatus: {
        setParsingStatus(SortingListsStatus);
        QTimer::singleShot(50, this, SLOT(sortStartTimes()));
        break;
    }
    case SortingEndsStatus: {
        setParsingStatus(SortingListsStatus);
        QTimer::singleShot(50, this, SLOT(sortEndTimes()));
        break;
    }
    case SortingListsStatus: {
        setParsingStatus(ComputingLevelsStatus);
        QTimer::singleShot(50, this, SLOT(computeLevels()));
        break;
    }
    case ComputingLevelsStatus: {
        setParsingStatus(CompilingStatisticsStatus);
        QTimer::singleShot(50, this, SLOT(compileStatistics()));
        break;
    }
    case CompilingStatisticsStatus: {
        linkEndsToStarts();
        setParsingStatus(DoneStatus);
        emit dataReady();
        break;
    }
    default: break;
    }

}

void QmlProfilerEventList::setParsingStatus(ParsingStatus ps)
{
    if (d->m_parsingStatus != ps) {
        d->m_parsingStatus = ps;
        emit parsingStatusChanged();
    }
}

ParsingStatus QmlProfilerEventList::getParsingStatus() const
{
    return d->m_parsingStatus;
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
    // continue postprocess
    postProcess();
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

int QmlProfilerEventList::count() const
{
    return d->m_startTimeSortedList.count();
}

////////////////////////////////////////////////////////////////////////////////////


void QmlProfilerEventList::save(const QString &filename)
{
    if (count() == 0) {
        emit error(tr("No data to save"));
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        emit error(tr("Could not open %1 for writing").arg(filename));
        return;
    }

    QXmlStreamWriter stream(&file);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    stream.writeStartElement("trace");

    stream.writeStartElement("eventData");
    foreach (const QmlEventData *eventData, d->m_eventDescriptions.values()) {
        stream.writeStartElement("event");
        stream.writeAttribute("index", QString::number(d->m_eventDescriptions.keys().indexOf(eventData->location)));
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
        stream.writeAttribute("eventIndex", QString::number(d->m_eventDescriptions.keys().indexOf(rangedEvent.description->location)));
        stream.writeEndElement();
    }
    stream.writeEndElement(); // eventList

    stream.writeEndElement(); // trace
    stream.writeEndDocument();

    file.close();
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

    setParsingStatus(GettingDataStatus);

    // erase current
    clear();

    QHash <int, QmlEventData *> descriptionBuffer;
    QmlEventData *currentEvent = 0;
    bool startTimesAreSorted = true;

    QXmlStreamReader stream(&file);

    while (!stream.atEnd() && !stream.hasError()) {
        QXmlStreamReader::TokenType token = stream.readNext();
        QString elementName = stream.name().toString();
        switch (token) {
            case QXmlStreamReader::StartDocument :  continue;
            case QXmlStreamReader::StartElement : {
                if (elementName == "event") {
                    QXmlStreamAttributes attributes = stream.attributes();
                    if (attributes.hasAttribute("index")) {
                        int ndx = attributes.value("index").toString().toInt();
                        if (!descriptionBuffer.value(ndx))
                            descriptionBuffer[ndx] = new QmlEventData;
                        currentEvent = descriptionBuffer[ndx];
                    }
                    break;
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
                            descriptionBuffer[ndx] = new QmlEventData();
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

                // the remaining are eventdata elements
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
                break;
            }
            case QXmlStreamReader::EndElement : {
                if (elementName == "event")
                    currentEvent = 0;
                break;
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

    // move the buffered data to the details cache
    foreach (QmlEventData *desc, descriptionBuffer.values()) {
        QString location = QString("%1:%2:%3").arg(QString::number(desc->eventType), desc->displayname, desc->details);
        desc->location = location;
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

    emit countChanged();

    setParsingStatus(SortingEndsStatus);

    descriptionBuffer.clear();

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


} // namespace QmlJsDebugClient
