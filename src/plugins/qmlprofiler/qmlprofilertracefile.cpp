/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlprofilertracefile.h"

#include <utils/qtcassert.h>

#include <QIODevice>
#include <QStringList>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QStack>
#include <QDebug>
#include <QFile>
#include <QBuffer>
#include <QDataStream>

namespace QmlProfiler {
namespace Internal {

const char PROFILER_FILE_VERSION[] = "1.02";

static const char *RANGE_TYPE_STRINGS[] = {
    "Painting",
    "Compiling",
    "Creating",
    "Binding",
    "HandlingSignal",
    "Javascript"
};

Q_STATIC_ASSERT(sizeof(RANGE_TYPE_STRINGS) == MaximumRangeType * sizeof(const char *));

static const char *MESSAGE_STRINGS[] = {
    // So far only pixmap and scenegraph are used. The others are padding.
    "Event",
    "RangeStart",
    "RangeData",
    "RangeLocation",
    "RangeEnd",
    "Complete",
    "PixmapCache",
    "SceneGraph",
    "MemoryAllocation",
    "DebugMessage"
};

Q_STATIC_ASSERT(sizeof(MESSAGE_STRINGS) == MaximumMessage * sizeof(const char *));

#define _(X) QLatin1String(X)

//
// "be strict in your output but tolerant in your inputs"
//

static QPair<Message, RangeType> qmlTypeAsEnum(const QString &typeString)
{
    QPair<Message, RangeType> ret(MaximumMessage, MaximumRangeType);

    for (int i = 0; i < MaximumMessage; ++i) {
        if (typeString == _(MESSAGE_STRINGS[i])) {
            ret.first = static_cast<Message>(i);
            break;
        }
    }

    for (int i = 0; i < MaximumRangeType; ++i) {
        if (typeString == _(RANGE_TYPE_STRINGS[i])) {
            ret.second = static_cast<RangeType>(i);
            break;
        }
    }

    if (ret.first == MaximumMessage && ret.second == MaximumRangeType) {
        bool isNumber = false;
        int type = typeString.toUInt(&isNumber);
        if (isNumber && type < MaximumRangeType)
            // Allow saving ranges as numbers, but not messages.
            ret.second = static_cast<RangeType>(type);
    }

    return ret;
}

static QString qmlTypeAsString(Message message, RangeType rangeType)
{
    if (rangeType < MaximumRangeType)
        return _(RANGE_TYPE_STRINGS[rangeType]);
    else if (message < MaximumMessage)
        return _(MESSAGE_STRINGS[message]);
    else
        return QString::number((int)rangeType);
}


QmlProfilerFileReader::QmlProfilerFileReader(QObject *parent) :
    QObject(parent),
    m_traceStart(-1),
    m_traceEnd(-1),
    m_future(0),
    m_loadedFeatures(0)
{
    static int meta[] = {
        qRegisterMetaType<QVector<QmlEvent> >(),
        qRegisterMetaType<QVector<QmlEventType> >(),
        qRegisterMetaType<QVector<QmlNote> >()
    };
    Q_UNUSED(meta);
}

void QmlProfilerFileReader::setFuture(QFutureInterface<void> *future)
{
    m_future = future;
    if (m_future) {
        m_future->setProgressRange(0, 1000);
        m_future->setProgressValue(0);
    }
}

void QmlProfilerFileReader::loadQtd(QIODevice *device)
{
    QXmlStreamReader stream(device);

    bool validVersion = true;

    while (validVersion && !stream.atEnd() && !stream.hasError() && !isCanceled()) {
        QXmlStreamReader::TokenType token = stream.readNext();
        const QStringRef elementName = stream.name();
        switch (token) {
        case QXmlStreamReader::StartDocument :  continue;
        case QXmlStreamReader::StartElement : {
            if (elementName == _("trace")) {
                QXmlStreamAttributes attributes = stream.attributes();
                if (attributes.hasAttribute(_("version")))
                    validVersion = attributes.value(_("version")) == _(PROFILER_FILE_VERSION);
                else
                    validVersion = false;
                if (attributes.hasAttribute(_("traceStart")))
                    m_traceStart = attributes.value(_("traceStart")).toLongLong();
                if (attributes.hasAttribute(_("traceEnd")))
                    m_traceEnd = attributes.value(_("traceEnd")).toLongLong();
            }

            if (elementName == _("eventData")) {
                loadEventTypes(stream);
                emit typesLoaded(m_eventTypes);
                break;
            }

            if (elementName == _("profilerDataModel")) {
                loadEvents(stream);
                break;
            }

            if (elementName == _("noteData")) {
                loadNotes(stream);
                emit notesLoaded(m_notes);
                break;
            }

            break;
        }
        default: break;
        }
    }

    if (isCanceled())
        emit canceled();
    else if (stream.hasError())
        emit error(tr("Error while parsing trace data file: %1").arg(stream.errorString()));
    else
        emit success();
}

void QmlProfilerFileReader::loadQzt(QIODevice *device)
{
    QDataStream stream(device);
    stream.setVersion(QDataStream::Qt_5_5);

    QByteArray magic;
    stream >> magic;
    if (magic != QByteArray("QMLPROFILER")) {
        emit error(tr("Invalid magic: %1").arg(QLatin1String(magic)));
        return;
    }

    qint32 dataStreamVersion;
    stream >> dataStreamVersion;

    if (dataStreamVersion > QDataStream::Qt_DefaultCompiledVersion) {
        emit error(tr("Unknown data stream version: %1").arg(dataStreamVersion));
        return;
    }
    stream.setVersion(dataStreamVersion);

    stream >> m_traceStart >> m_traceEnd;

    QBuffer buffer;
    QDataStream bufferStream(&buffer);
    bufferStream.setVersion(dataStreamVersion);
    QByteArray data;
    updateProgress(device);

    if (!isCanceled()) {
        stream >> data;
        buffer.setData(qUncompress(data));
        buffer.open(QIODevice::ReadOnly);
        bufferStream >> m_eventTypes;
        buffer.close();
        emit typesLoaded(m_eventTypes);
        updateProgress(device);
    }

    if (!isCanceled()) {
        stream >> data;
        buffer.setData(qUncompress(data));
        buffer.open(QIODevice::ReadOnly);
        bufferStream >> m_notes;
        buffer.close();
        emit notesLoaded(m_notes);
        updateProgress(device);
    }

    QVector<QmlEvent> eventBuffer;
    while (!stream.atEnd() && !isCanceled()) {
        stream >> data;
        buffer.setData(qUncompress(data));
        buffer.open(QIODevice::ReadOnly);
        while (!buffer.atEnd() && !isCanceled()) {
            QmlEvent event;
            bufferStream >> event;
            if (bufferStream.status() == QDataStream::Ok) {
                if (event.typeIndex() >= m_eventTypes.length()) {
                    emit error(tr("Invalid type index %1").arg(event.typeIndex()));
                    return;
                }
                m_loadedFeatures |= (1ULL << m_eventTypes[event.typeIndex()].feature());
                if (event.timestamp() < 0)
                    event.setTimestamp(0);
            } else if (bufferStream.status() == QDataStream::ReadPastEnd) {
                break; // Apparently EOF is a character so we end up here after the last event.
            } else if (bufferStream.status() == QDataStream::ReadCorruptData) {
                emit error(tr("Corrupt data before position %1.").arg(device->pos()));
                return;
            } else {
                Q_UNREACHABLE();
            }
            eventBuffer.append(event);
        }
        emit qmlEventsLoaded(eventBuffer);
        eventBuffer.clear();
        buffer.close();
        updateProgress(device);
    }

    if (isCanceled()) {
        emit canceled();
    } else {
        emit qmlEventsLoaded(eventBuffer);
        emit success();
    }
}

quint64 QmlProfilerFileReader::loadedFeatures() const
{
    return m_loadedFeatures;
}

void QmlProfilerFileReader::loadEventTypes(QXmlStreamReader &stream)
{
    QTC_ASSERT(stream.name() == _("eventData"), return);

    int typeIndex = -1;

    QPair<Message, RangeType> messageAndRange(MaximumMessage, MaximumRangeType);
    int detailType = -1;
    QString displayName;
    QString data;
    QString filename;
    int line = 0, column = 0;

    auto clearType = [&](){
        messageAndRange = QPair<Message, RangeType>(MaximumMessage, MaximumRangeType);
        detailType = -1;
        displayName.clear();
        data.clear();
        filename.clear();
        line = column = 0;
    };

    while (!stream.atEnd() && !stream.hasError() && !isCanceled()) {

        QXmlStreamReader::TokenType token = stream.readNext();
        const QStringRef elementName = stream.name();

        switch (token) {
        case QXmlStreamReader::StartElement: {
            if (elementName == _("event")) {
                updateProgress(stream.device());
                clearType();

                const QXmlStreamAttributes attributes = stream.attributes();
                if (attributes.hasAttribute(_("index"))) {
                    typeIndex = attributes.value(_("index")).toInt();
                } else {
                    // ignore event
                    typeIndex = -1;
                }
                break;
            }

            stream.readNext();
            if (stream.tokenType() != QXmlStreamReader::Characters)
                break;

            const QString readData = stream.text().toString();

            if (elementName == _("displayname")) {
                displayName = readData;
                break;
            }

            if (elementName == _("type")) {
                messageAndRange = qmlTypeAsEnum(readData);
                break;
            }

            if (elementName == _("filename")) {
                filename = readData;
                break;
            }

            if (elementName == _("line")) {
                line = readData.toInt();
                break;
            }

            if (elementName == _("column")) {
                column = readData.toInt();
                break;
            }

            if (elementName == _("details")) {
                data = readData;
                break;
            }

            if (elementName == _("animationFrame")) {
                detailType = readData.toInt();
                // new animation frames used to be saved as ranges of range type Painting with
                // binding type 4 (which was called "AnimationFrame" to make everything even more
                // confusing), even though they clearly aren't ranges. Convert that to something
                // sane here.
                if (detailType == 4) {
                    messageAndRange = QPair<Message, RangeType>(Event, MaximumRangeType);
                    detailType = AnimationFrame;
                }
            }

            if (elementName == _("bindingType") ||
                    elementName == _("cacheEventType") ||
                    elementName == _("sgEventType") ||
                    elementName == _("memoryEventType") ||
                    elementName == _("mouseEvent") ||
                    elementName == _("keyEvent") ||
                    elementName == _("level")) {
                detailType = readData.toInt();
                break;
            }

            break;
        }
        case QXmlStreamReader::EndElement: {
            if (elementName == _("event")) {
                if (typeIndex >= 0) {
                    if (typeIndex >= m_eventTypes.size())
                        m_eventTypes.resize(typeIndex + 1);
                    QmlEventType type(messageAndRange.first, messageAndRange.second, detailType,
                                      QmlEventLocation(filename, line, column), data, displayName);
                    m_eventTypes[typeIndex] = type;
                    ProfileFeature feature = type.feature();
                    if (feature != MaximumProfileFeature)
                        m_loadedFeatures |= (1ULL << static_cast<uint>(feature));
                }
                break;
            }

            if (elementName == _("eventData")) {
                // done reading eventData
                return;
            }
            break;
        }
        default: break;
        } // switch
    }
}

class EventList
{
public:
    void addEvent(const QmlEvent &event);
    void addRange(const QmlEvent &start, const QmlEvent &end);

    QVector<QmlEvent> finalize();

private:
    struct QmlRange {
        QmlEvent begin;
        QmlEvent end;
    };

    QList<QmlRange> ranges; // We are going to do a lot of takeFirst() on this.
};

void EventList::addEvent(const QmlEvent &event)
{
    ranges.append({event, QmlEvent()});
}

void EventList::addRange(const QmlEvent &start, const QmlEvent &end)
{
    ranges.append({start, end});
}

QVector<QmlEvent> EventList::finalize()
{
    std::sort(ranges.begin(), ranges.end(), [](const QmlRange &a, const QmlRange &b) {
        if (a.begin.timestamp() < b.begin.timestamp())
            return true;
        if (a.begin.timestamp() > b.begin.timestamp())
            return false;

        // If the start times are equal. Sort the one with the greater end time first, so that
        // the nesting is retained.
        return (a.end.timestamp() > b.end.timestamp());
    });

    QList<QmlEvent> ends;
    QVector<QmlEvent> result;
    while (!ranges.isEmpty()) {
        // This is more expensive than just iterating, but we don't want to double the already
        // high memory footprint.
        QmlRange range = ranges.takeFirst();
        while (!ends.isEmpty() && ends.last().timestamp() <= range.begin.timestamp())
            result.append(ends.takeLast());

        result.append(range.begin);
        if (range.end.isValid()) {
            auto it = ends.end();
            for (auto begin = ends.begin(); it != begin;) {
                if ((--it)->timestamp() >= range.end.timestamp()) {
                    ++it;
                    break;
                }
            }
            ends.insert(it, range.end);
        }
    }
    while (!ends.isEmpty())
        result.append(ends.takeLast());

    return result;
}

void QmlProfilerFileReader::loadEvents(QXmlStreamReader &stream)
{
    QTC_ASSERT(stream.name() == _("profilerDataModel"), return);
    EventList events;

    while (!stream.atEnd() && !stream.hasError() && !isCanceled()) {

        QXmlStreamReader::TokenType token = stream.readNext();
        const QStringRef elementName = stream.name();

        switch (token) {
        case QXmlStreamReader::StartElement: {
            if (elementName == _("range")) {
                updateProgress(stream.device());
                QmlEvent event;

                const QXmlStreamAttributes attributes = stream.attributes();
                if (!attributes.hasAttribute(_("startTime"))
                        || !attributes.hasAttribute(_("eventIndex"))) {
                    // ignore incomplete entry
                    continue;
                }

                const qint64 timestamp = attributes.value(_("startTime")).toLongLong();
                event.setTimestamp(timestamp > 0 ? timestamp : 0);
                event.setTypeIndex(attributes.value(_("eventIndex")).toInt());

                if (attributes.hasAttribute(_("duration"))) {
                    event.setRangeStage(RangeStart);
                    QmlEvent rangeEnd(event);
                    rangeEnd.setRangeStage(RangeEnd);
                    const qint64 duration = attributes.value(_("duration")).toLongLong();
                    rangeEnd.setTimestamp(event.timestamp() + (duration > 0 ? duration : 0));
                    events.addRange(event, rangeEnd);
                } else {
                    // attributes for special events
                    if (attributes.hasAttribute(_("framerate")))
                        event.setNumber<qint32>(0, attributes.value(_("framerate")).toInt());
                    if (attributes.hasAttribute(_("animationcount")))
                        event.setNumber<qint32>(1, attributes.value(_("animationcount")).toInt());
                    if (attributes.hasAttribute(_("thread")))
                        event.setNumber<qint32>(2, attributes.value(_("thread")).toInt());
                    if (attributes.hasAttribute(_("width")))
                        event.setNumber<qint32>(0, attributes.value(_("width")).toInt());
                    if (attributes.hasAttribute(_("height")))
                        event.setNumber<qint32>(1, attributes.value(_("height")).toInt());
                    if (attributes.hasAttribute(_("refCount")))
                        event.setNumber<qint32>(2, attributes.value(_("refCount")).toInt());
                    if (attributes.hasAttribute(_("amount")))
                        event.setNumber<qint64>(0, attributes.value(_("amount")).toLongLong());
                    if (attributes.hasAttribute(_("timing1")))
                        event.setNumber<qint64>(0, attributes.value(_("timing1")).toLongLong());
                    if (attributes.hasAttribute(_("timing2")))
                        event.setNumber<qint64>(1, attributes.value(_("timing2")).toLongLong());
                    if (attributes.hasAttribute(_("timing3")))
                        event.setNumber<qint64>(2, attributes.value(_("timing3")).toLongLong());
                    if (attributes.hasAttribute(_("timing4")))
                        event.setNumber<qint64>(3, attributes.value(_("timing4")).toLongLong());
                    if (attributes.hasAttribute(_("timing5")))
                        event.setNumber<qint64>(4, attributes.value(_("timing5")).toLongLong());
                    if (attributes.hasAttribute(_("type")))
                        event.setNumber<qint32>(0, attributes.value(_("type")).toInt());
                    if (attributes.hasAttribute(_("data1")))
                        event.setNumber<qint32>(1, attributes.value(_("data1")).toInt());
                    if (attributes.hasAttribute(_("data2")))
                        event.setNumber<qint32>(2, attributes.value(_("data2")).toInt());
                    if (attributes.hasAttribute(_("text")))
                        event.setString(attributes.value(_("text")).toString());

                    events.addEvent(event);
                }
            }
            break;
        }
        case QXmlStreamReader::EndElement: {
            if (elementName == _("profilerDataModel")) {
                // done reading profilerDataModel
                emit qmlEventsLoaded(events.finalize());
                return;
            }
            break;
        }
        default: break;
        } // switch
    }
}

void QmlProfilerFileReader::loadNotes(QXmlStreamReader &stream)
{
    QmlNote currentNote;
    while (!stream.atEnd() && !stream.hasError() && !isCanceled()) {

        QXmlStreamReader::TokenType token = stream.readNext();
        const QStringRef elementName = stream.name();

        switch (token) {
        case QXmlStreamReader::StartElement: {
            if (elementName == _("note")) {
                updateProgress(stream.device());
                QXmlStreamAttributes attrs = stream.attributes();
                int collapsedRow = attrs.hasAttribute(_("collapsedRow")) ?
                            attrs.value(_("collapsedRow")).toInt() : -1;

                currentNote = QmlNote(attrs.value(_("eventIndex")).toInt(),
                                      collapsedRow,
                                      attrs.value(_("startTime")).toLongLong(),
                                      attrs.value(_("duration")).toLongLong());
            }
            break;
        }
        case QXmlStreamReader::Characters: {
            currentNote.setText(stream.text().toString());
            break;
        }
        case QXmlStreamReader::EndElement: {
            if (elementName == _("note")) {
                m_notes.append(currentNote);
            } else if (elementName == _("noteData")) {
                return;
            }
            break;
        }
        default:
            break;
        }
    }
}

void QmlProfilerFileReader::updateProgress(QIODevice *device)
{
    if (!m_future)
        return;

    m_future->setProgressValue(device->pos() * 1000 / device->size());
}

bool QmlProfilerFileReader::isCanceled() const
{
    return m_future && m_future->isCanceled();
}

QmlProfilerFileWriter::QmlProfilerFileWriter(QObject *parent) :
    QObject(parent),
    m_startTime(0),
    m_endTime(0),
    m_measuredTime(0),
    m_future(0)
{
}

void QmlProfilerFileWriter::setTraceTime(qint64 startTime, qint64 endTime, qint64 measuredTime)
{
    m_startTime = startTime;
    m_endTime = endTime;
    m_measuredTime = measuredTime;
}

void QmlProfilerFileWriter::setData(const QmlProfilerModelManager *model)
{
    m_modelManager = model;
}

void QmlProfilerFileWriter::setNotes(const QVector<QmlNote> &notes)
{
    m_notes = notes;
}

void QmlProfilerFileWriter::setFuture(QFutureInterface<void> *future)
{
    m_future = future;
    if (m_future) {
        m_future->setProgressRange(0, ProgressTotal);
        m_future->setProgressValue(0);
    }
}

void QmlProfilerFileWriter::saveQtd(QIODevice *device)
{
    QXmlStreamWriter stream(device);

    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    stream.writeStartElement(_("trace"));
    stream.writeAttribute(_("version"), _(PROFILER_FILE_VERSION));

    stream.writeAttribute(_("traceStart"), QString::number(m_startTime));
    stream.writeAttribute(_("traceEnd"), QString::number(m_endTime));

    stream.writeStartElement(_("eventData"));
    stream.writeAttribute(_("totalTime"), QString::number(m_measuredTime));
    const QVector<QmlEventType> &eventTypes = m_modelManager->eventTypes();
    for (int typeIndex = 0, end = eventTypes.length(); typeIndex < end && !isCanceled();
         ++typeIndex) {

        const QmlEventType &type = eventTypes[typeIndex];

        stream.writeStartElement(_("event"));
        stream.writeAttribute(_("index"), QString::number(typeIndex));
        stream.writeTextElement(_("displayname"), type.displayName());
        stream.writeTextElement(_("type"), qmlTypeAsString(type.message(), type.rangeType()));
        const QmlEventLocation location(type.location());
        if (!location.filename().isEmpty()) {
            stream.writeTextElement(_("filename"), location.filename());
            stream.writeTextElement(_("line"), QString::number(location.line()));
            stream.writeTextElement(_("column"), QString::number(location.column()));
        }

        if (!type.data().isEmpty())
            stream.writeTextElement(_("details"), type.data());

        if (type.rangeType() == Binding) {
            stream.writeTextElement(_("bindingType"), QString::number(type.detailType()));
        } else if (type.message() == Event) {
            switch (type.detailType()) {
            case AnimationFrame:
                stream.writeTextElement(_("animationFrame"), QString::number(type.detailType()));
                break;
            case Key:
                stream.writeTextElement(_("keyEvent"), QString::number(type.detailType()));
                break;
            case Mouse:
                stream.writeTextElement(_("mouseEvent"), QString::number(type.detailType()));
                break;
            default:
                break;
            }
        } else if (type.message() == PixmapCacheEvent) {
            stream.writeTextElement(_("cacheEventType"), QString::number(type.detailType()));
        } else if (type.message() == SceneGraphFrame) {
            stream.writeTextElement(_("sgEventType"), QString::number(type.detailType()));
        } else if (type.message() == MemoryAllocation) {
            stream.writeTextElement(_("memoryEventType"), QString::number(type.detailType()));
        } else if (type.message() == DebugMessage) {
            stream.writeTextElement(_("level"), QString::number(type.detailType()));
        }
        stream.writeEndElement();
    }
    updateProgress(ProgressTypes);
    stream.writeEndElement(); // eventData

    if (!isCanceled()) {
        stream.writeStartElement(_("profilerDataModel"));

        QStack<QmlEvent> stack;
        const bool success = m_modelManager->replayEvents(
                    -1, -1, [this, &stack, &stream](const QmlEvent &event,
                    const QmlEventType &type) {
            if (isCanceled())
                return;

            if (type.rangeType() != MaximumRangeType && event.rangeStage() == RangeStart) {
                stack.push(event);
                return;
            }

            stream.writeStartElement(_("range"));
            if (type.rangeType() != MaximumRangeType && event.rangeStage() == RangeEnd) {
                QmlEvent start = stack.pop();
                stream.writeAttribute(_("startTime"), QString::number(start.timestamp()));
                stream.writeAttribute(_("duration"),
                                      QString::number(event.timestamp() - start.timestamp()));
            } else {
                stream.writeAttribute(_("startTime"), QString::number(event.timestamp()));
            }

            stream.writeAttribute(_("eventIndex"), QString::number(event.typeIndex()));

            if (type.message() == Event) {
                if (type.detailType() == AnimationFrame) {
                    // special: animation event
                    stream.writeAttribute(_("framerate"), QString::number(event.number<qint32>(0)));
                    stream.writeAttribute(_("animationcount"),
                                          QString::number(event.number<qint32>(1)));
                    stream.writeAttribute(_("thread"), QString::number(event.number<qint32>(2)));
                } else if (type.detailType() == Key || type.detailType() == Mouse) {
                    // special: input event
                    stream.writeAttribute(_("type"), QString::number(event.number<qint32>(0)));
                    stream.writeAttribute(_("data1"), QString::number(event.number<qint32>(1)));
                    stream.writeAttribute(_("data2"), QString::number(event.number<qint32>(2)));
                }
            }

            // special: pixmap cache event
            if (type.message() == PixmapCacheEvent) {
                if (type.detailType() == PixmapSizeKnown) {
                    stream.writeAttribute(_("width"), QString::number(event.number<qint32>(0)));
                    stream.writeAttribute(_("height"), QString::number(event.number<qint32>(1)));
                }

                if (type.detailType() == PixmapReferenceCountChanged
                        || type.detailType() == PixmapCacheCountChanged)
                    stream.writeAttribute(_("refCount"), QString::number(event.number<qint32>(2)));
            }

            if (type.message() == SceneGraphFrame) {
                // special: scenegraph frame events
                for (int i = 0; i < 5; ++i) {
                    qint64 number = event.number<qint64>(i);
                    if (number <= 0)
                        continue;
                    stream.writeAttribute(QString::fromLatin1("timing%1").arg(i + 1),
                                          QString::number(number));
                }
            }

            // special: memory allocation event
            if (type.message() == MemoryAllocation)
                stream.writeAttribute(_("amount"), QString::number(event.number<qint64>(0)));

            if (type.message() == DebugMessage)
                stream.writeAttribute(_("text"), event.string());

            stream.writeEndElement();

            // Update the progress roughly every 4k events. It doesn't have to be precise.
            if ((event.timestamp() & 0xfff) == 0)
                updateProgress(event.timestamp());
        });
        if (!success) {
            emit error(tr("Could not re-read events from temporary trace file. Saving failed."));
            return;
        }

        stream.writeEndElement(); // profilerDataModel
    }

    if (!isCanceled()) {
        stream.writeStartElement(_("noteData"));
        for (int noteIndex = 0; noteIndex < m_notes.size() && !isCanceled(); ++noteIndex) {

            const QmlNote &note = m_notes[noteIndex];
            stream.writeStartElement(_("note"));
            stream.writeAttribute(_("startTime"), QString::number(note.startTime()));
            stream.writeAttribute(_("duration"), QString::number(note.duration()));
            stream.writeAttribute(_("eventIndex"), QString::number(note.typeIndex()));
            stream.writeAttribute(_("collapsedRow"), QString::number(note.collapsedRow()));
            stream.writeCharacters(note.text());
            stream.writeEndElement(); // note
        }
        stream.writeEndElement(); // noteData
        updateProgress(ProgressNotes);
    }

    stream.writeEndElement(); // trace
    stream.writeEndDocument();

    if (isCanceled()) {
        emit canceled();
    } else if (stream.hasError()) {
        emit error(tr("Error writing trace file."));
    } else {
        emit success();
    }
}

void QmlProfilerFileWriter::saveQzt(QFile *file)
{
    QDataStream stream(file);
    stream.setVersion(QDataStream::Qt_5_5);
    stream << QByteArray("QMLPROFILER");
    stream << static_cast<qint32>(QDataStream::Qt_DefaultCompiledVersion);
    stream.setVersion(QDataStream::Qt_DefaultCompiledVersion);

    stream << m_startTime << m_endTime;

    QBuffer buffer;
    QDataStream bufferStream(&buffer);
    buffer.open(QIODevice::WriteOnly);

    if (!isCanceled()) {
        bufferStream << m_modelManager->eventTypes();
        stream << qCompress(buffer.data());
        buffer.close();
        buffer.buffer().clear();
        updateProgress(ProgressTypes);
    }

    if (!isCanceled()) {
        buffer.open(QIODevice::WriteOnly);
        bufferStream << m_notes;
        stream << qCompress(buffer.data());
        buffer.close();
        buffer.buffer().clear();
        updateProgress(ProgressNotes);
    }

    if (!isCanceled()) {
        buffer.open(QIODevice::WriteOnly);
        const bool success = m_modelManager->replayEvents(
                    -1, -1, [this, &stream, &buffer, &bufferStream](const QmlEvent &event,
                                                                    const QmlEventType &type) {
            Q_UNUSED(type);
            bufferStream << event;
            // 32MB buffer should be plenty for efficient compression
            if (buffer.data().length() > (1 << 25)) {
                stream << qCompress(buffer.data());
                buffer.close();
                buffer.buffer().clear();
                if (isCanceled())
                    return;
                buffer.open(QIODevice::WriteOnly);
                updateProgress(event.timestamp());
            }
        });
        if (!success) {
            emit error(tr("Could not re-read events from temporary trace file. Saving failed."));
            return;
        }
    }

    if (isCanceled()) {
        emit canceled();
    } else {
        stream << qCompress(buffer.data());
        buffer.close();
        buffer.buffer().clear();
        updateProgress(m_endTime);
        emit success();
    }
}

void QmlProfilerFileWriter::updateProgress(qint64 timestamp)
{
    if (!m_future)
        return;

    if (timestamp < 0) {
        m_future->setProgressValue(m_future->progressValue() - timestamp);
    } else {
        m_future->setProgressValue(m_future->progressValue()
                                   + float(m_endTime - timestamp) / float(m_endTime - m_startTime)
                                   * ProgressEvents);
    }
}

bool QmlProfilerFileWriter::isCanceled() const
{
    return m_future && m_future->isCanceled();
}

} // namespace Internal
} // namespace QmlProfiler
