#include "qv8profilerdatamodel.h"
#include "qmlprofilerdatamodel.h"

#include <QStringList>

using namespace QmlDebug;

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(QmlProfiler::Internal::QV8EventData, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(QmlProfiler::Internal::QV8EventSub, Q_MOVABLE_TYPE);
QT_END_NAMESPACE

namespace QmlProfiler {
namespace Internal {

QV8EventData &QV8EventData::operator=(const QV8EventData &ref)
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

class QV8ProfilerDataModel::QV8ProfilerDataModelPrivate
{
public:
    QV8ProfilerDataModelPrivate(QV8ProfilerDataModel *qq) {Q_UNUSED(qq);}

    QmlProfilerDataModel *qmlProfilerDataModel;

    void clearV8RootEvent();
    void collectV8Statistics();

    QHash<QString, QV8EventData *> v8EventHash;
    QHash<int, QV8EventData *> v8parents;
    QV8EventData v8RootEvent;
    qint64 v8MeasuredTime;
};

QV8ProfilerDataModel::QV8ProfilerDataModel(QObject *parent,
                                           QmlProfilerDataModel *profilerDataModel)
    : QObject(parent), d(new QV8ProfilerDataModelPrivate(this))
{
    d->v8MeasuredTime = 0;
    d->clearV8RootEvent();
    d->qmlProfilerDataModel = profilerDataModel;
}

QV8ProfilerDataModel::~QV8ProfilerDataModel()
{
    delete d;
}

void QV8ProfilerDataModel::clear()
{
    qDeleteAll(d->v8EventHash.values());
    d->v8EventHash.clear();
    d->v8parents.clear();
    d->clearV8RootEvent();
    d->v8MeasuredTime = 0;
}

bool QV8ProfilerDataModel::isEmpty() const
{
    return d->v8EventHash.isEmpty();
}

QV8EventData *QV8ProfilerDataModel::v8EventDescription(int eventId) const
{
    foreach (QV8EventData *event, d->v8EventHash.values()) {
        if (event->eventId == eventId)
            return event;
    }
    return 0;
}

qint64 QV8ProfilerDataModel::v8MeasuredTime() const
{
    return d->v8MeasuredTime;
}

QList<QV8EventData *> QV8ProfilerDataModel::getV8Events() const
{
    return d->v8EventHash.values();
}

void QV8ProfilerDataModel::addV8Event(int depth,
                                      const QString &function,
                                      const QString &filename,
                                      int lineNumber,
                                      double totalTime,
                                      double selfTime)
{
    QString displayName = filename.mid(filename.lastIndexOf(QLatin1Char('/')) + 1) +
            QLatin1Char(':') + QString::number(lineNumber);
    QString hashStr = QmlProfilerDataModel::getHashStringForV8Event(displayName, function);

    d->qmlProfilerDataModel->setState(QmlProfilerDataModel::AcquiringData);

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
    if (depth > 0 && d->v8parents.contains(depth-1)) {
        parentEvent = d->v8parents.value(depth-1);
    }

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

void QV8ProfilerDataModel::collectV8Statistics()
{
    d->collectV8Statistics();
}

void QV8ProfilerDataModel::QV8ProfilerDataModelPrivate::collectV8Statistics()
{
    if (!v8EventHash.isEmpty()) {
        double totalTimes = v8MeasuredTime;
        double selfTimes = 0;
        foreach (QV8EventData *v8event, v8EventHash.values()) {
            selfTimes += v8event->selfTime;
        }

        // prevent divisions by 0
        if (totalTimes == 0)
            totalTimes = 1;
        if (selfTimes == 0)
            selfTimes = 1;

        // insert root event in eventlist
        // the +1 ns is to get it on top of the sorted list
        v8RootEvent.totalTime = v8MeasuredTime + 1;
        v8RootEvent.selfTime = 0;

        QString rootEventHash = QmlProfilerDataModel::getHashStringForV8Event(
                    QmlProfilerDataModel::rootEventName(),
                    QmlProfilerDataModel::rootEventDescription());
        QV8EventData *v8RootEventPointer = v8EventHash[rootEventHash];
        if (v8RootEventPointer) {
            v8RootEvent = *v8RootEventPointer;
        } else {
            v8EventHash[rootEventHash] = new QV8EventData;
            *v8EventHash[rootEventHash] = v8RootEvent;
        }

        foreach (QV8EventData *v8event, v8EventHash.values()) {
            v8event->totalPercent = v8event->totalTime * 100.0 / totalTimes;
            v8event->selfPercent = v8event->selfTime * 100.0 / selfTimes;
        }

        int index = 0;
        foreach (QV8EventData *v8event, v8EventHash.values()) {
            v8event->eventId = index++;
        }
        v8RootEvent.eventId = v8EventHash[rootEventHash]->eventId;
    } else {
        // On empty data, still add a fake root event
        clearV8RootEvent();
        v8RootEvent.totalPercent = 100;
        QString rootEventHash = QmlProfilerDataModel::getHashStringForV8Event(
                    QmlProfilerDataModel::rootEventName(),
                    QmlProfilerDataModel::rootEventDescription());
        v8EventHash[rootEventHash] = new QV8EventData;
        *v8EventHash[rootEventHash] = v8RootEvent;
    }
}

void QV8ProfilerDataModel::QV8ProfilerDataModelPrivate::clearV8RootEvent()
{
    v8RootEvent.displayName = QmlProfilerDataModel::rootEventName();
    v8RootEvent.eventHashStr = QmlProfilerDataModel::rootEventName();
    v8RootEvent.functionName = QmlProfilerDataModel::rootEventDescription();
    v8RootEvent.line = -1;
    v8RootEvent.totalTime = 0;
    v8RootEvent.totalPercent = 0;
    v8RootEvent.selfTime = 0;
    v8RootEvent.selfPercent = 0;
    v8RootEvent.eventId = -1;

    qDeleteAll(v8RootEvent.parentHash.values());
    qDeleteAll(v8RootEvent.childrenHash.values());
    v8RootEvent.parentHash.clear();
    v8RootEvent.childrenHash.clear();
}

void QV8ProfilerDataModel::save(QXmlStreamWriter &stream)
{
    stream.writeStartElement("v8profile"); // v8 profiler output
    stream.writeAttribute("totalTime", QString::number(d->v8MeasuredTime));
    foreach (QV8EventData *v8event, d->v8EventHash.values()) {
        stream.writeStartElement("event");
        stream.writeAttribute("index",
                              QString::number(
                                  d->v8EventHash.keys().indexOf(
                                      v8event->eventHashStr)));
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
                parentTimes << QString::number(v8child->totalTime);
            }

            stream.writeAttribute("list", childrenIndexes.join(QString(", ")));
            stream.writeAttribute("childrenTimes", childrenTimes.join(QString(", ")));
            stream.writeAttribute("parentTimes", parentTimes.join(QString(", ")));
            stream.writeEndElement();
        }
        stream.writeEndElement();
    }
    stream.writeEndElement(); // v8 profiler output
}

void QV8ProfilerDataModel::load(QXmlStreamReader &stream)
{
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
    if (attributes.hasAttribute("totalTime"))
        d->v8MeasuredTime = attributes.value("totalTime").toString().toDouble();

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

                if (elementName == "childrenEvents") {
                    QXmlStreamAttributes attributes = stream.attributes();
                    int eventIndex = v8eventBuffer.key(v8event);
                    if (attributes.hasAttribute("list")) {
                        // store for later parsing (we haven't read all the events yet)
                        childrenIndexes[eventIndex] = attributes.value("list").toString();
                    }
                    if (attributes.hasAttribute("childrenTimes")) {
                        childrenTimes[eventIndex] =
                                attributes.value("childrenTimes").toString();
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
            break;
        }
        case QXmlStreamReader::EndElement : {
            if (elementName == "v8profile") {
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
    foreach (int parentIndex, childrenIndexes.keys()) {
        QStringList childrenStrings = childrenIndexes.value(parentIndex).split(",");
        QStringList childrenTimesStrings = childrenTimes.value(parentIndex).split(", ");
        QStringList parentTimesStrings = parentTimes.value(parentIndex).split(", ");
        for (int ndx = 0; ndx < childrenStrings.count(); ndx++) {
            int childIndex = childrenStrings[ndx].toInt();
            if (v8eventBuffer.value(childIndex)) {
                QV8EventSub *newChild = new QV8EventSub(v8eventBuffer[childIndex]);
                QV8EventSub *newParent = new QV8EventSub(v8eventBuffer[parentIndex]);
                if (childrenTimesStrings.count() > ndx) {
                    newChild->totalTime = childrenTimesStrings[ndx].toDouble();
                }
                if (parentTimesStrings.count() > ndx) {
                    newParent->totalTime = parentTimesStrings[ndx].toDouble();
                }
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
    foreach (QV8EventData *storedV8Event, v8eventBuffer.values()) {
        storedV8Event->eventHashStr =
                QmlProfilerDataModel::getHashStringForV8Event(
                    storedV8Event->displayName, storedV8Event->functionName);
        d->v8EventHash[storedV8Event->eventHashStr] = storedV8Event;
    }

    d->collectV8Statistics();

}

} // namespace Internal
} // namespace QmlProfiler
