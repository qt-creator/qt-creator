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
#include <QDebug>

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
    else if (message != MaximumMessage)
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
}

void QmlProfilerFileReader::setFuture(QFutureInterface<void> *future)
{
    m_future = future;
}

bool QmlProfilerFileReader::load(QIODevice *device)
{
    if (m_future) {
        m_future->setProgressRange(0, 1000);
        m_future->setProgressValue(0);
    }

    QXmlStreamReader stream(device);

    bool validVersion = true;

    while (validVersion && !stream.atEnd() && !stream.hasError()) {
        if (isCanceled())
            return false;
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
                break;
            }

            if (elementName == _("profilerDataModel")) {
                loadEvents(stream);
                break;
            }

            if (elementName == _("noteData")) {
                loadNotes(stream);
                break;
            }

            break;
        }
        default: break;
        }
    }

    if (stream.hasError()) {
        emit error(tr("Error while parsing trace data file: %1").arg(stream.errorString()));
        return false;
    } else {
        emit success();
        return true;
    }
}

quint64 QmlProfilerFileReader::loadedFeatures() const
{
    return m_loadedFeatures;
}

ProfileFeature featureFromType(const QmlEventType &type) {
    if (type.rangeType < MaximumRangeType)
        return featureFromRangeType(type.rangeType);

    switch (type.message) {
    case Event:
        switch (type.detailType) {
        case AnimationFrame:
            return ProfileAnimations;
        case Key:
        case Mouse:
            return ProfileInputEvents;
        default:
            return MaximumProfileFeature;
        }
    case PixmapCacheEvent:
        return ProfilePixmapCache;
    case SceneGraphFrame:
        return ProfileSceneGraph;
    case MemoryAllocation:
        return ProfileMemory;
    case DebugMessage:
        return ProfileDebugMessages;
    default:
        return MaximumProfileFeature;
    }
}

void QmlProfilerFileReader::loadEventTypes(QXmlStreamReader &stream)
{
    QTC_ASSERT(stream.name() == _("eventData"), return);

    int typeIndex = -1;
    QmlEventType type = {
            QString(), // displayname
            QmlEventLocation(),
            MaximumMessage,
            Painting, // type
            QmlBinding,  // bindingType, set for backwards compatibility
            QString(), // details
    };
    const QmlEventType defaultEvent = type;

    while (!stream.atEnd() && !stream.hasError()) {
        if (isCanceled())
            return;

        QXmlStreamReader::TokenType token = stream.readNext();
        const QStringRef elementName = stream.name();

        switch (token) {
        case QXmlStreamReader::StartElement: {
            if (elementName == _("event")) {
                progress(stream.device());
                type = defaultEvent;

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
                type.displayName = readData;
                break;
            }

            if (elementName == _("type")) {
                QPair<Message, RangeType> enums = qmlTypeAsEnum(readData);
                type.message = enums.first;
                type.rangeType = enums.second;
                break;
            }

            if (elementName == _("filename")) {
                type.location.filename = readData;
                break;
            }

            if (elementName == _("line")) {
                type.location.line = readData.toInt();
                break;
            }

            if (elementName == _("column")) {
                type.location.column = readData.toInt();
                break;
            }

            if (elementName == _("details")) {
                type.data = readData;
                break;
            }

            if (elementName == _("animationFrame")) {
                type.detailType = readData.toInt();
                // new animation frames used to be saved as ranges of range type Painting with
                // binding type 4 (which was called "AnimationFrame" to make everything even more
                // confusing), even though they clearly aren't ranges. Convert that to something
                // sane here.
                if (type.detailType == 4) {
                    type.message = Event;
                    type.rangeType = MaximumRangeType;
                    type.detailType = AnimationFrame;
                }
            }

            if (elementName == _("bindingType") ||
                    elementName == _("cacheEventType") ||
                    elementName == _("sgEventType") ||
                    elementName == _("memoryEventType") ||
                    elementName == _("mouseEvent") ||
                    elementName == _("keyEvent") ||
                    elementName == _("level")) {
                type.detailType = readData.toInt();
                break;
            }

            break;
        }
        case QXmlStreamReader::EndElement: {
            if (elementName == _("event")) {
                if (typeIndex >= 0) {
                    if (typeIndex >= m_eventTypes.size())
                        m_eventTypes.resize(typeIndex + 1);
                    m_eventTypes[typeIndex] = type;
                    ProfileFeature feature = featureFromType(type);
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

void QmlProfilerFileReader::loadEvents(QXmlStreamReader &stream)
{
    QTC_ASSERT(stream.name() == _("profilerDataModel"), return);

    while (!stream.atEnd() && !stream.hasError()) {
        if (isCanceled())
            return;

        QXmlStreamReader::TokenType token = stream.readNext();
        const QStringRef elementName = stream.name();

        switch (token) {
        case QXmlStreamReader::StartElement: {
            if (elementName == _("range")) {
                progress(stream.device());
                QmlEvent event(0, 0, -1, 0, 0, 0, 0, 0);

                const QXmlStreamAttributes attributes = stream.attributes();
                if (!attributes.hasAttribute(_("startTime"))
                        || !attributes.hasAttribute(_("eventIndex"))) {
                    // ignore incomplete entry
                    continue;
                }

                event.setTimestamp(attributes.value(_("startTime")).toLongLong());
                if (attributes.hasAttribute(_("duration")))
                    event.setDuration(attributes.value(_("duration")).toLongLong());

                // attributes for special events
                if (attributes.hasAttribute(_("framerate")))
                    event.setNumericData(0, attributes.value(_("framerate")).toLongLong());
                if (attributes.hasAttribute(_("animationcount")))
                    event.setNumericData(1, attributes.value(_("animationcount")).toLongLong());
                if (attributes.hasAttribute(_("thread")))
                    event.setNumericData(2, attributes.value(_("thread")).toLongLong());
                if (attributes.hasAttribute(_("width")))
                    event.setNumericData(0, attributes.value(_("width")).toLongLong());
                if (attributes.hasAttribute(_("height")))
                    event.setNumericData(1, attributes.value(_("height")).toLongLong());
                if (attributes.hasAttribute(_("refCount")))
                    event.setNumericData(2, attributes.value(_("refCount")).toLongLong());
                if (attributes.hasAttribute(_("amount")))
                    event.setNumericData(0, attributes.value(_("amount")).toLongLong());
                if (attributes.hasAttribute(_("timing1")))
                    event.setNumericData(0, attributes.value(_("timing1")).toLongLong());
                if (attributes.hasAttribute(_("timing2")))
                    event.setNumericData(1, attributes.value(_("timing2")).toLongLong());
                if (attributes.hasAttribute(_("timing3")))
                    event.setNumericData(2, attributes.value(_("timing3")).toLongLong());
                if (attributes.hasAttribute(_("timing4")))
                    event.setNumericData(3, attributes.value(_("timing4")).toLongLong());
                if (attributes.hasAttribute(_("timing5")))
                    event.setNumericData(4, attributes.value(_("timing5")).toLongLong());
                if (attributes.hasAttribute(_("type")))
                    event.setNumericData(0, attributes.value(_("type")).toLongLong());
                if (attributes.hasAttribute(_("data1")))
                    event.setNumericData(1, attributes.value(_("data1")).toLongLong());
                if (attributes.hasAttribute(_("data2")))
                    event.setNumericData(2, attributes.value(_("data2")).toLongLong());
                if (attributes.hasAttribute(_("text")))
                    event.setStringData(attributes.value(_("text")).toString());

                event.setTypeIndex(attributes.value(_("eventIndex")).toInt());

                m_events.append(event);
            }
            break;
        }
        case QXmlStreamReader::EndElement: {
            if (elementName == _("profilerDataModel")) {
                // done reading profilerDataModel
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
    while (!stream.atEnd() && !stream.hasError()) {
        if (isCanceled())
            return;

        QXmlStreamReader::TokenType token = stream.readNext();
        const QStringRef elementName = stream.name();

        switch (token) {
        case QXmlStreamReader::StartElement: {
            if (elementName == _("note")) {
                progress(stream.device());
                QXmlStreamAttributes attrs = stream.attributes();
                currentNote.startTime = attrs.value(_("startTime")).toLongLong();
                currentNote.duration = attrs.value(_("duration")).toLongLong();
                currentNote.typeIndex = attrs.value(_("eventIndex")).toInt();
            }
            break;
        }
        case QXmlStreamReader::Characters: {
            currentNote.text = stream.text().toString();
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

void QmlProfilerFileReader::progress(QIODevice *device)
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

void QmlProfilerFileWriter::setData(const QVector<QmlEventType> &types,
                                         const QVector<QmlEvent> &events)
{
    m_eventTypes = types;
    m_events = events;
}

void QmlProfilerFileWriter::setNotes(const QVector<QmlNote> &notes)
{
    m_notes = notes;
}

void QmlProfilerFileWriter::setFuture(QFutureInterface<void> *future)
{
    m_future = future;
}

void QmlProfilerFileWriter::save(QIODevice *device)
{
    if (m_future) {
        m_future->setProgressRange(0,
            qMax(m_eventTypes.size() + m_events.size() + m_notes.size(), 1));
        m_future->setProgressValue(0);
        m_newProgressValue = 0;
    }

    QXmlStreamWriter stream(device);

    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    stream.writeStartElement(_("trace"));
    stream.writeAttribute(_("version"), _(PROFILER_FILE_VERSION));

    stream.writeAttribute(_("traceStart"), QString::number(m_startTime));
    stream.writeAttribute(_("traceEnd"), QString::number(m_endTime));

    stream.writeStartElement(_("eventData"));
    stream.writeAttribute(_("totalTime"), QString::number(m_measuredTime));

    for (int typeIndex = 0; typeIndex < m_eventTypes.size(); ++typeIndex) {
        if (isCanceled())
            return;

        const QmlEventType &type = m_eventTypes[typeIndex];

        stream.writeStartElement(_("event"));
        stream.writeAttribute(_("index"), QString::number(typeIndex));
        stream.writeTextElement(_("displayname"), type.displayName);
        stream.writeTextElement(_("type"), qmlTypeAsString(type.message, type.rangeType));
        if (!type.location.filename.isEmpty()) {
            stream.writeTextElement(_("filename"), type.location.filename);
            stream.writeTextElement(_("line"), QString::number(type.location.line));
            stream.writeTextElement(_("column"), QString::number(type.location.column));
        }

        if (!type.data.isEmpty())
            stream.writeTextElement(_("details"), type.data);

        if (type.rangeType == Binding) {
            stream.writeTextElement(_("bindingType"), QString::number(type.detailType));
        } else if (type.message == Event) {
            switch (type.detailType) {
            case AnimationFrame:
                stream.writeTextElement(_("animationFrame"), QString::number(type.detailType));
                break;
            case Key:
                stream.writeTextElement(_("keyEvent"), QString::number(type.detailType));
                break;
            case Mouse:
                stream.writeTextElement(_("mouseEvent"), QString::number(type.detailType));
                break;
            default:
                break;
            }
        } else if (type.message == PixmapCacheEvent) {
            stream.writeTextElement(_("cacheEventType"), QString::number(type.detailType));
        } else if (type.message == SceneGraphFrame) {
            stream.writeTextElement(_("sgEventType"), QString::number(type.detailType));
        } else if (type.message == MemoryAllocation) {
            stream.writeTextElement(_("memoryEventType"), QString::number(type.detailType));
        } else if (type.message == DebugMessage) {
            stream.writeTextElement(_("level"), QString::number(type.detailType));
        }
        stream.writeEndElement();
        incrementProgress();
    }
    stream.writeEndElement(); // eventData

    stream.writeStartElement(_("profilerDataModel"));

    for (int rangeIndex = 0; rangeIndex < m_events.size(); ++rangeIndex) {
        if (isCanceled())
            return;

        const QmlEvent &event = m_events[rangeIndex];

        stream.writeStartElement(_("range"));
        stream.writeAttribute(_("startTime"), QString::number(event.timestamp()));
        if (event.duration() > 0) // no need to store duration of instantaneous events
            stream.writeAttribute(_("duration"), QString::number(event.duration()));
        stream.writeAttribute(_("eventIndex"), QString::number(event.typeIndex()));

        const QmlEventType &type = m_eventTypes[event.typeIndex()];

        if (type.message == Event) {
            if (type.detailType == AnimationFrame) {
                // special: animation event
                stream.writeAttribute(_("framerate"), QString::number(event.numericData(0)));
                stream.writeAttribute(_("animationcount"), QString::number(event.numericData(1)));
                stream.writeAttribute(_("thread"), QString::number(event.numericData(2)));
            } else if (type.detailType == Key || type.detailType == Mouse) {
                // special: input event
                stream.writeAttribute(_("type"), QString::number(event.numericData(0)));
                stream.writeAttribute(_("data1"), QString::number(event.numericData(1)));
                stream.writeAttribute(_("data2"), QString::number(event.numericData(2)));
            }
        }

        // special: pixmap cache event
        if (type.message == PixmapCacheEvent) {
            if (type.detailType == PixmapSizeKnown) {
                stream.writeAttribute(_("width"), QString::number(event.numericData(0)));
                stream.writeAttribute(_("height"), QString::number(event.numericData(1)));
            }

            if (type.detailType == PixmapReferenceCountChanged ||
                    type.detailType == PixmapCacheCountChanged)
                stream.writeAttribute(_("refCount"), QString::number(event.numericData(2)));
        }

        if (type.message == SceneGraphFrame) {
            // special: scenegraph frame events
            if (event.numericData(0) > 0)
                stream.writeAttribute(_("timing1"), QString::number(event.numericData(0)));
            if (event.numericData(1) > 0)
                stream.writeAttribute(_("timing2"), QString::number(event.numericData(1)));
            if (event.numericData(2) > 0)
                stream.writeAttribute(_("timing3"), QString::number(event.numericData(2)));
            if (event.numericData(3) > 0)
                stream.writeAttribute(_("timing4"), QString::number(event.numericData(3)));
            if (event.numericData(4) > 0)
                stream.writeAttribute(_("timing5"), QString::number(event.numericData(4)));
        }

        // special: memory allocation event
        if (type.message == MemoryAllocation)
            stream.writeAttribute(_("amount"), QString::number(event.numericData(0)));

        if (type.message == DebugMessage)
            stream.writeAttribute(_("text"), event.stringData());

        stream.writeEndElement();
        incrementProgress();
    }
    stream.writeEndElement(); // profilerDataModel

    stream.writeStartElement(_("noteData"));
    for (int noteIndex = 0; noteIndex < m_notes.size(); ++noteIndex) {
        if (isCanceled())
            return;

        const QmlNote &note = m_notes[noteIndex];
        stream.writeStartElement(_("note"));
        stream.writeAttribute(_("startTime"), QString::number(note.startTime));
        stream.writeAttribute(_("duration"), QString::number(note.duration));
        stream.writeAttribute(_("eventIndex"), QString::number(note.typeIndex));
        stream.writeCharacters(note.text);
        stream.writeEndElement(); // note
        incrementProgress();
    }
    stream.writeEndElement(); // noteData

    if (isCanceled())
        return;

    stream.writeEndElement(); // trace
    stream.writeEndDocument();
}

void QmlProfilerFileWriter::incrementProgress()
{
    if (!m_future)
        return;

    m_newProgressValue++;
    if (float(m_newProgressValue - m_future->progressValue())
            / float(m_future->progressMaximum() - m_future->progressMinimum()) >= 0.01) {
        m_future->setProgressValue(m_newProgressValue);
    }
}

bool QmlProfilerFileWriter::isCanceled() const
{
    return m_future && m_future->isCanceled();
}

} // namespace Internal
} // namespace QmlProfiler
