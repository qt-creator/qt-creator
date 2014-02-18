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

#include "qv8profilerdatamodel.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerdetailsrewriter.h"
#include "qmlprofilerbasemodel_p.h"
#include <utils/qtcassert.h>

#include <QStringList>

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(QmlProfiler::QV8ProfilerDataModel::QV8EventData, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(QmlProfiler::QV8ProfilerDataModel::QV8EventSub, Q_MOVABLE_TYPE);
QT_END_NAMESPACE

namespace QmlProfiler {

typedef QHash <QString, QV8ProfilerDataModel::QV8EventSub *> EventHash;

static EventHash cloneEventHash(const EventHash &src)
{
    EventHash result;
    const EventHash::ConstIterator cend = src.constEnd();
    for (EventHash::ConstIterator it = src.constBegin(); it != cend; ++it)
        result.insert(it.key(), new QV8ProfilerDataModel::QV8EventSub(it.value()));
    return result;
}

QV8ProfilerDataModel::QV8EventData &QV8ProfilerDataModel::QV8EventData::operator=(const QV8EventData &ref)
{
    if (this == &ref)
        return *this;

    displayName = ref.displayName;
    eventHashStr = ref.eventHashStr;
    filename = ref.filename;
    functionName = ref.functionName;
    line = ref.line;
    totalTime = ref.totalTime;
    totalPercent = ref.totalPercent;
    selfTime = ref.selfTime;
    SelfTimeInPercent = ref.SelfTimeInPercent;
    eventId = ref.eventId;

    qDeleteAll(parentHash);
    parentHash = cloneEventHash(ref.parentHash);

    qDeleteAll(childrenHash);
    childrenHash = cloneEventHash(ref.childrenHash);

    return *this;
}

QV8ProfilerDataModel::QV8EventData::QV8EventData()
{
    line = -1;
    eventId = -1;
    totalTime = 0;
    selfTime = 0;
    totalPercent = 0;
    SelfTimeInPercent = 0;
}

QV8ProfilerDataModel::QV8EventData::~QV8EventData()
{
    qDeleteAll(parentHash.values());
    parentHash.clear();
    qDeleteAll(childrenHash.values());
    childrenHash.clear();
}

class QV8ProfilerDataModel::QV8ProfilerDataModelPrivate :
        public QmlProfilerBaseModel::QmlProfilerBaseModelPrivate
{
public:
    QV8ProfilerDataModelPrivate(QV8ProfilerDataModel *qq) :
            QmlProfilerBaseModel::QmlProfilerBaseModelPrivate(qq) {}

    QHash<QString, QV8EventData *> v8EventHash;
    QList<QV8EventData *> pendingRewrites;
    QHash<int, QV8EventData *> v8parents;
    QV8EventData v8RootEvent;
    qint64 v8MeasuredTime;

private:
    Q_DECLARE_PUBLIC(QV8ProfilerDataModel)
};

QV8ProfilerDataModel::QV8ProfilerDataModel(Utils::FileInProjectFinder *fileFinder,
                                           QmlProfilerModelManager *parent)
    : QmlProfilerBaseModel(fileFinder, parent, new QV8ProfilerDataModelPrivate(this))
{
    Q_D(QV8ProfilerDataModel);
    d->v8MeasuredTime = 0;
    clearV8RootEvent();
}

void QV8ProfilerDataModel::clear()
{
    Q_D(QV8ProfilerDataModel);
    qDeleteAll(d->v8EventHash.values());
    d->v8EventHash.clear();
    d->v8parents.clear();
    clearV8RootEvent();
    d->v8MeasuredTime = 0;
    d->pendingRewrites.clear();

    QmlProfilerBaseModel::clear();
}

bool QV8ProfilerDataModel::isEmpty() const
{
    Q_D(const QV8ProfilerDataModel);
    return d->v8EventHash.isEmpty();
}

QV8ProfilerDataModel::QV8EventData *QV8ProfilerDataModel::v8EventDescription(int eventId) const
{
    Q_D(const QV8ProfilerDataModel);
    foreach (QV8EventData *event, d->v8EventHash) {
        if (event->eventId == eventId)
            return event;
    }
    return 0;
}

qint64 QV8ProfilerDataModel::v8MeasuredTime() const
{
    Q_D(const QV8ProfilerDataModel);
    return d->v8MeasuredTime;
}

QList<QV8ProfilerDataModel::QV8EventData *> QV8ProfilerDataModel::getV8Events() const
{
    Q_D(const QV8ProfilerDataModel);
    return d->v8EventHash.values();
}

QString getHashStringForV8Event(const QString &displayName, const QString &function)
{
    return QString::fromLatin1("%1:%2").arg(displayName, function);
}

void QV8ProfilerDataModel::addV8Event(int depth,
                                      const QString &function,
                                      const QString &filename,
                                      int lineNumber,
                                      double totalTime,
                                      double selfTime)
{
    Q_D(QV8ProfilerDataModel);
    QString displayName = filename.mid(filename.lastIndexOf(QLatin1Char('/')) + 1) +
            QLatin1Char(':') + QString::number(lineNumber);
    QString hashStr = getHashStringForV8Event(displayName, function);

    // time is given in milliseconds, but internally we store it in microseconds
    totalTime *= 1e6;
    selfTime *= 1e6;

    // accumulate information
    QV8EventData *eventData = d->v8EventHash[hashStr];
    if (!eventData) {
        eventData = new QV8EventData;
        eventData->displayName = displayName;
        eventData->eventHashStr = hashStr;
        eventData->filename = filename;
        eventData->functionName = function;
        eventData->line = lineNumber;
        eventData->totalTime = totalTime;
        eventData->selfTime = selfTime;
        d->v8EventHash[hashStr] = eventData;
    } else {
        eventData->totalTime += totalTime;
        eventData->selfTime += selfTime;
    }
    d->v8parents[depth] = eventData;

    QV8EventData *parentEvent = 0;
    if (depth == 0) {
        parentEvent = &d->v8RootEvent;
        d->v8MeasuredTime += totalTime;
    }
    if (depth > 0 && d->v8parents.contains(depth-1))
        parentEvent = d->v8parents.value(depth-1);

    if (parentEvent != 0) {
        if (!eventData->parentHash.contains(parentEvent->eventHashStr)) {
            QV8EventSub *newParentSub = new QV8EventSub(parentEvent);
            newParentSub->totalTime = totalTime;

            eventData->parentHash.insert(parentEvent->eventHashStr, newParentSub);
        } else {
            QV8EventSub *newParentSub = eventData->parentHash.value(parentEvent->eventHashStr);
            newParentSub->totalTime += totalTime;
        }

        if (!parentEvent->childrenHash.contains(eventData->eventHashStr)) {
            QV8EventSub *newChildSub = new QV8EventSub(eventData);
            newChildSub->totalTime = totalTime;

            parentEvent->childrenHash.insert(eventData->eventHashStr, newChildSub);
        } else {
            QV8EventSub *newChildSub = parentEvent->childrenHash.value(eventData->eventHashStr);
            newChildSub->totalTime += totalTime;
        }
    }

}

void QV8ProfilerDataModel::detailsChanged(int requestId, const QString &newString)
{
    Q_D(QV8ProfilerDataModel);
    QTC_ASSERT(requestId < d->pendingRewrites.count(), return);
    d->pendingRewrites[requestId]->filename = newString;
}

void QV8ProfilerDataModel::detailsDone()
{
    Q_D(QV8ProfilerDataModel);
    d->pendingRewrites.clear();
    QmlProfilerBaseModel::detailsDone();
}

void QV8ProfilerDataModel::complete()
{
    Q_D(QV8ProfilerDataModel);
    if (!d->v8EventHash.isEmpty()) {
        double totalTimes = d->v8MeasuredTime;
        double selfTimes = 0;
        foreach (const QV8EventData *v8event, d->v8EventHash) {
            selfTimes += v8event->selfTime;
        }

        // prevent divisions by 0
        if (totalTimes == 0)
            totalTimes = 1;
        if (selfTimes == 0)
            selfTimes = 1;

        // insert root event in eventlist
        // the +1 ns is to get it on top of the sorted list
        d->v8RootEvent.totalTime = d->v8MeasuredTime + 1;
        d->v8RootEvent.selfTime = 0;

        QString rootEventHash = getHashStringForV8Event(
                    tr("<program>"),
                    tr("Main Program"));
        QV8EventData *v8RootEventPointer = d->v8EventHash[rootEventHash];
        if (v8RootEventPointer) {
            d->v8RootEvent = *v8RootEventPointer;
        } else {
            d->v8EventHash[rootEventHash] = new QV8EventData;
            *(d->v8EventHash[rootEventHash]) = d->v8RootEvent;
        }

        foreach (QV8EventData *v8event, d->v8EventHash) {
            v8event->totalPercent = v8event->totalTime * 100.0 / totalTimes;
            v8event->SelfTimeInPercent = v8event->selfTime * 100.0 / selfTimes;
        }

        int index = d->pendingRewrites.size();
        foreach (QV8EventData *v8event, d->v8EventHash.values()) {
            v8event->eventId = index++;
            d->pendingRewrites << v8event;
            d->detailsRewriter->requestDetailsForLocation(index,
                    QmlDebug::QmlEventLocation(v8event->filename, v8event->line, 1));
        }

        d->v8RootEvent.eventId = d->v8EventHash[rootEventHash]->eventId;
    } else {
        // On empty data, still add a fake root event
        clearV8RootEvent();
    }

    QmlProfilerBaseModel::complete();
}

void QV8ProfilerDataModel::clearV8RootEvent()
{
    Q_D(QV8ProfilerDataModel);
    d->v8RootEvent.displayName = tr("<program>");
    d->v8RootEvent.eventHashStr = tr("<program>");
    d->v8RootEvent.functionName = tr("Main Program");

    d->v8RootEvent.line = -1;
    d->v8RootEvent.totalTime = 0;
    d->v8RootEvent.totalPercent = 0;
    d->v8RootEvent.selfTime = 0;
    d->v8RootEvent.SelfTimeInPercent = 0;
    d->v8RootEvent.eventId = -1;

    qDeleteAll(d->v8RootEvent.parentHash.values());
    qDeleteAll(d->v8RootEvent.childrenHash.values());
    d->v8RootEvent.parentHash.clear();
    d->v8RootEvent.childrenHash.clear();
}

void QV8ProfilerDataModel::save(QXmlStreamWriter &stream)
{
    Q_D(QV8ProfilerDataModel);
    stream.writeStartElement(QLatin1String("v8profile")); // v8 profiler output
    stream.writeAttribute(QLatin1String("totalTime"), QString::number(d->v8MeasuredTime));
    foreach (const QV8EventData *v8event, d->v8EventHash) {
        stream.writeStartElement(QLatin1String("event"));
        stream.writeAttribute(QLatin1String("index"),
                              QString::number(
                                  d->v8EventHash.keys().indexOf(
                                      v8event->eventHashStr)));
        stream.writeTextElement(QLatin1String("displayname"), v8event->displayName);
        stream.writeTextElement(QLatin1String("functionname"), v8event->functionName);
        if (!v8event->filename.isEmpty()) {
            stream.writeTextElement(QLatin1String("filename"), v8event->filename);
            stream.writeTextElement(QLatin1String("line"), QString::number(v8event->line));
        }
        stream.writeTextElement(QLatin1String("totalTime"), QString::number(v8event->totalTime));
        stream.writeTextElement(QLatin1String("selfTime"), QString::number(v8event->selfTime));
        if (!v8event->childrenHash.isEmpty()) {
            stream.writeStartElement(QLatin1String("childrenEvents"));
            QStringList childrenIndexes;
            QStringList childrenTimes;
            QStringList parentTimes;
            foreach (const QV8EventSub *v8child, v8event->childrenHash) {
                childrenIndexes << QString::number(v8child->reference->eventId);
                childrenTimes << QString::number(v8child->totalTime);
                parentTimes << QString::number(v8child->totalTime);
            }

            stream.writeAttribute(QLatin1String("list"), childrenIndexes.join(QLatin1String(", ")));
            stream.writeAttribute(QLatin1String("childrenTimes"), childrenTimes.join(QLatin1String(", ")));
            stream.writeAttribute(QLatin1String("parentTimes"), parentTimes.join(QLatin1String(", ")));
            stream.writeEndElement();
        }
        stream.writeEndElement();
    }
    stream.writeEndElement(); // v8 profiler output
}

void QV8ProfilerDataModel::load(QXmlStreamReader &stream)
{
    Q_D(QV8ProfilerDataModel);
    QHash <int, QV8EventData *> v8eventBuffer;
    QHash <int, QString> childrenIndexes;
    QHash <int, QString> childrenTimes;
    QHash <int, QString> parentTimes;
    QV8EventData *v8event = 0;

    // time computation
    d->v8MeasuredTime = 0;
    double cumulatedV8Time = 0;

    // get the v8 time
    QXmlStreamAttributes attributes = stream.attributes();
    if (attributes.hasAttribute(QLatin1String("totalTime")))
        d->v8MeasuredTime = attributes.value(QLatin1String("totalTime")).toString().toDouble();

    while (!stream.atEnd() && !stream.hasError()) {
        QXmlStreamReader::TokenType token = stream.readNext();
        const QStringRef elementName = stream.name();
        switch (token) {
        case QXmlStreamReader::StartDocument :  continue;
        case QXmlStreamReader::StartElement : {
                if (elementName == QLatin1String("event")) {
                    QXmlStreamAttributes attributes = stream.attributes();
                    if (attributes.hasAttribute(QLatin1String("index"))) {
                        int ndx = attributes.value(QLatin1String("index")).toString().toInt();
                        if (!v8eventBuffer.value(ndx))
                            v8eventBuffer[ndx] = new QV8EventData;
                        v8event = v8eventBuffer[ndx];
                    } else {
                        v8event = 0;
                    }
                    break;
                }

                if (!v8event)
                    break;

                if (elementName == QLatin1String("childrenEvents")) {
                    QXmlStreamAttributes attributes = stream.attributes();
                    int eventIndex = v8eventBuffer.key(v8event);
                    if (attributes.hasAttribute(QLatin1String("list"))) {
                        // store for later parsing (we haven't read all the events yet)
                        childrenIndexes[eventIndex] = attributes.value(QLatin1String("list")).toString();
                    }
                    if (attributes.hasAttribute(QLatin1String("childrenTimes"))) {
                        childrenTimes[eventIndex] =
                                attributes.value(QLatin1String("childrenTimes")).toString();
                    }
                    if (attributes.hasAttribute(QLatin1String("parentTimes")))
                        parentTimes[eventIndex] = attributes.value(QLatin1String("parentTimes")).toString();
                }

                stream.readNext();
                if (stream.tokenType() != QXmlStreamReader::Characters)
                    break;
                QString readData = stream.text().toString();

                if (elementName == QLatin1String("displayname")) {
                    v8event->displayName = readData;
                    break;
                }

                if (elementName == QLatin1String("functionname")) {
                    v8event->functionName = readData;
                    break;
                }

                if (elementName == QLatin1String("filename")) {
                    v8event->filename = readData;
                    break;
                }

                if (elementName == QLatin1String("line")) {
                    v8event->line = readData.toInt();
                    break;
                }

                if (elementName == QLatin1String("totalTime")) {
                    v8event->totalTime = readData.toDouble();
                    cumulatedV8Time += v8event->totalTime;
                    break;
                }

                if (elementName == QLatin1String("selfTime")) {
                    v8event->selfTime = readData.toDouble();
                    break;
                }
            break;
        }
        case QXmlStreamReader::EndElement : {
            if (elementName == QLatin1String("v8profile")) {
                // done reading the v8 profile data
                break;
            }
        }
        default: break;
        }
    }

    // backwards compatibility
    if (d->v8MeasuredTime == 0)
        d->v8MeasuredTime = cumulatedV8Time;

    // find v8events' children and parents
    typedef QHash <int, QString>::ConstIterator ChildIndexConstIt;

    const ChildIndexConstIt icend = childrenIndexes.constEnd();
    for (ChildIndexConstIt it = childrenIndexes.constBegin(); it != icend; ++it) {
        const int parentIndex = it.key();
        const QStringList childrenStrings = it.value().split(QLatin1Char(','));
        QStringList childrenTimesStrings = childrenTimes.value(parentIndex).split(QLatin1String(", "));
        QStringList parentTimesStrings = parentTimes.value(parentIndex).split(QLatin1String(", "));
        for (int ndx = 0; ndx < childrenStrings.count(); ndx++) {
            int childIndex = childrenStrings[ndx].toInt();
            if (v8eventBuffer.value(childIndex)) {
                QV8EventSub *newChild = new QV8EventSub(v8eventBuffer[childIndex]);
                QV8EventSub *newParent = new QV8EventSub(v8eventBuffer[parentIndex]);
                if (childrenTimesStrings.count() > ndx)
                    newChild->totalTime = childrenTimesStrings[ndx].toDouble();
                if (parentTimesStrings.count() > ndx)
                    newParent->totalTime = parentTimesStrings[ndx].toDouble();
                v8eventBuffer[parentIndex]->childrenHash.insert(
                            newChild->reference->displayName,
                            newChild);
                v8eventBuffer[childIndex]->parentHash.insert(
                            newParent->reference->displayName,
                            newParent);
            }
        }
    }
    // store v8 events
    foreach (QV8EventData *storedV8Event, v8eventBuffer) {
        storedV8Event->eventHashStr =
                getHashStringForV8Event(
                    storedV8Event->displayName, storedV8Event->functionName);
        d->v8EventHash[storedV8Event->eventHashStr] = storedV8Event;
    }

    complete();
}

} // namespace QmlProfiler
