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

#include "qmlprofilertracefile.h"

#include <utils/qtcassert.h>

#include <QIODevice>
#include <QStringList>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDebug>

// import QmlEventType, QmlBindingType enums, QmlEventLocation
using namespace QmlDebug;


const char PROFILER_FILE_VERSION[] = "1.02";

static const char *RANGE_TYPE_STRINGS[] = {
    "Painting",
    "Compiling",
    "Creating",
    "Binding",
    "HandlingSignal",
    "Javascript"
};

Q_STATIC_ASSERT(sizeof(RANGE_TYPE_STRINGS) == QmlDebug::MaximumRangeType * sizeof(const char *));

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
    "MemoryAllocation"
};

Q_STATIC_ASSERT(sizeof(MESSAGE_STRINGS) == QmlDebug::MaximumMessage * sizeof(const char *));

#define _(X) QLatin1String(X)

//
// "be strict in your output but tolerant in your inputs"
//

namespace QmlProfiler {
namespace Internal {

static QPair<QmlDebug::Message, QmlDebug::RangeType> qmlTypeAsEnum(const QString &typeString)
{
    QPair<QmlDebug::Message, QmlDebug::RangeType> ret(QmlDebug::MaximumMessage,
                                                      QmlDebug::MaximumRangeType);

    for (int i = 0; i < QmlDebug::MaximumMessage; ++i) {
        if (typeString == _(MESSAGE_STRINGS[i])) {
            ret.first = static_cast<QmlDebug::Message>(i);
            break;
        }
    }

    for (int i = 0; i < QmlDebug::MaximumRangeType; ++i) {
        if (typeString == _(RANGE_TYPE_STRINGS[i])) {
            ret.second = static_cast<QmlDebug::RangeType>(i);
            break;
        }
    }

    if (ret.first == QmlDebug::MaximumMessage && ret.second == QmlDebug::MaximumRangeType) {
        bool isNumber = false;
        int type = typeString.toUInt(&isNumber);
        if (isNumber && type < QmlDebug::MaximumRangeType)
            // Allow saving ranges as numbers, but not messages.
            ret.second = static_cast<QmlDebug::RangeType>(type);
    }

    return ret;
}

static QString qmlTypeAsString(QmlDebug::Message message, QmlDebug::RangeType rangeType)
{
    if (rangeType < QmlDebug::MaximumRangeType)
        return _(RANGE_TYPE_STRINGS[rangeType]);
    else if (message != QmlDebug::MaximumMessage)
        return _(MESSAGE_STRINGS[message]);
    else
        return QString::number((int)rangeType);
}


QmlProfilerFileReader::QmlProfilerFileReader(QObject *parent) :
    QObject(parent),
    m_v8Model(0)
{
}

void QmlProfilerFileReader::setV8DataModel(QV8ProfilerDataModel *dataModel)
{
    m_v8Model = dataModel;
}

void QmlProfilerFileReader::setQmlDataModel(QmlProfilerDataModel *dataModel)
{
    m_qmlModel = dataModel;
}

bool QmlProfilerFileReader::load(QIODevice *device)
{
    QXmlStreamReader stream(device);

    bool validVersion = true;

    while (validVersion && !stream.atEnd() && !stream.hasError()) {
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
                     emit traceStartTime(attributes.value(_("traceStart")).toString().toLongLong());
                 if (attributes.hasAttribute(_("traceEnd")))
                     emit traceEndTime(attributes.value(_("traceEnd")).toString().toLongLong());
             }

             if (elementName == _("eventData")) {
                 loadEventData(stream);
                 break;
             }

             if (elementName == _("profilerDataModel")) {
                 loadProfilerDataModel(stream);
                 break;
             }

             if (elementName == _("v8profile")) {
                 if (m_v8Model)
                     m_v8Model->load(stream);
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
         m_qmlModel->setData(m_qmlEvents, m_ranges);
         return true;
     }
}

void QmlProfilerFileReader::loadEventData(QXmlStreamReader &stream)
{
    QTC_ASSERT(stream.name() == _("eventData"), return);

    QXmlStreamAttributes attributes = stream.attributes();

    int eventIndex = -1;
    QmlProfilerDataModel::QmlEventTypeData event = {
            QString(), // displayname
            QmlEventLocation(),
            MaximumMessage,
            Painting, // type
            QmlBinding,  // bindingType, set for backwards compatibility
            QString(), // details
    };
    const QmlProfilerDataModel::QmlEventTypeData defaultEvent = event;

    while (!stream.atEnd() && !stream.hasError()) {
        QXmlStreamReader::TokenType token = stream.readNext();
        const QStringRef elementName = stream.name();

        switch (token) {
        case QXmlStreamReader::StartElement: {
            if (elementName == _("event")) {
                event = defaultEvent;

                const QXmlStreamAttributes attributes = stream.attributes();
                if (attributes.hasAttribute(_("index"))) {
                    eventIndex = attributes.value(_("index")).toString().toInt();
                } else {
                    // ignore event
                    eventIndex = -1;
                }
                break;
            }

            stream.readNext();
            if (stream.tokenType() != QXmlStreamReader::Characters)
                break;

            const QString readData = stream.text().toString();

            if (elementName == _("displayname")) {
                event.displayName = readData;
                break;
            }

            if (elementName == _("type")) {
                QPair<QmlDebug::Message, QmlDebug::RangeType> enums = qmlTypeAsEnum(readData);
                event.message = enums.first;
                event.rangeType = enums.second;
                break;
            }

            if (elementName == _("filename")) {
                event.location.filename = readData;
                break;
            }

            if (elementName == _("line")) {
                event.location.line = readData.toInt();
                break;
            }

            if (elementName == _("column")) {
                event.location.column = readData.toInt();
                break;
            }

            if (elementName == _("details")) {
                event.data = readData;
                break;
            }

            if (elementName == _("animationFrame")) {
                event.detailType = readData.toInt();
                // new animation frames used to be saved as ranges of range type Painting with
                // binding type 4 (which was called "AnimationFrame" to make everything even more
                // confusing), even though they clearly aren't ranges. Convert that to something
                // sane here.
                if (event.detailType == 4) {
                    event.message = Event;
                    event.rangeType = MaximumRangeType;
                    event.detailType = AnimationFrame;
                }
            }

            if (elementName == _("bindingType") ||
                    elementName == _("cacheEventType") ||
                    elementName == _("sgEventType") ||
                    elementName == _("memoryEventType")) {
                event.detailType = readData.toInt();
                break;
            }

            break;
        }
        case QXmlStreamReader::EndElement: {
            if (elementName == _("event")) {
                if (eventIndex >= 0) {
                    if (eventIndex >= m_qmlEvents.size())
                        m_qmlEvents.resize(eventIndex + 1);
                    m_qmlEvents[eventIndex] = event;
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

void QmlProfilerFileReader::loadProfilerDataModel(QXmlStreamReader &stream)
{
    QTC_ASSERT(stream.name() == _("profilerDataModel"), return);

    while (!stream.atEnd() && !stream.hasError()) {
        QXmlStreamReader::TokenType token = stream.readNext();
        const QStringRef elementName = stream.name();

        switch (token) {
        case QXmlStreamReader::StartElement: {
            if (elementName == _("range")) {
                QmlProfilerDataModel::QmlEventData range = { -1, 0, 0, 0, 0, 0, 0, 0 };

                const QXmlStreamAttributes attributes = stream.attributes();
                if (!attributes.hasAttribute(_("startTime"))
                        || !attributes.hasAttribute(_("eventIndex"))) {
                    // ignore incomplete entry
                    continue;
                }

                range.startTime = attributes.value(_("startTime")).toString().toLongLong();
                if (attributes.hasAttribute(_("duration")))
                    range.duration = attributes.value(_("duration")).toString().toLongLong();

                // attributes for special events
                if (attributes.hasAttribute(_("framerate")))
                    range.numericData1 = attributes.value(_("framerate")).toString().toLongLong();
                if (attributes.hasAttribute(_("animationcount")))
                    range.numericData2 = attributes.value(_("animationcount")).toString().toLongLong();
                if (attributes.hasAttribute(_("thread")))
                    range.numericData3 = attributes.value(_("thread")).toString().toLongLong();
                if (attributes.hasAttribute(_("width")))
                    range.numericData1 = attributes.value(_("width")).toString().toLongLong();
                if (attributes.hasAttribute(_("height")))
                    range.numericData2 = attributes.value(_("height")).toString().toLongLong();
                if (attributes.hasAttribute(_("refCount")))
                    range.numericData3 = attributes.value(_("refCount")).toString().toLongLong();
                if (attributes.hasAttribute(_("amount")))
                    range.numericData1 = attributes.value(_("amount")).toString().toLongLong();
                if (attributes.hasAttribute(_("timing1")))
                    range.numericData1 = attributes.value(_("timing1")).toString().toLongLong();
                if (attributes.hasAttribute(_("timing2")))
                    range.numericData2 = attributes.value(_("timing2")).toString().toLongLong();
                if (attributes.hasAttribute(_("timing3")))
                    range.numericData3 = attributes.value(_("timing3")).toString().toLongLong();
                if (attributes.hasAttribute(_("timing4")))
                    range.numericData4 = attributes.value(_("timing4")).toString().toLongLong();
                if (attributes.hasAttribute(_("timing5")))
                    range.numericData5 = attributes.value(_("timing5")).toString().toLongLong();

                range.typeIndex = attributes.value(_("eventIndex")).toString().toInt();

                m_ranges.append(range);
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

QmlProfilerFileWriter::QmlProfilerFileWriter(QObject *parent) :
    QObject(parent),
    m_startTime(0),
    m_endTime(0),
    m_measuredTime(0),
    m_v8Model(0)
{
}

void QmlProfilerFileWriter::setTraceTime(qint64 startTime, qint64 endTime, qint64 measuredTime)
{
    m_startTime = startTime;
    m_endTime = endTime;
    m_measuredTime = measuredTime;
}

void QmlProfilerFileWriter::setV8DataModel(QV8ProfilerDataModel *dataModel)
{
    m_v8Model = dataModel;
}

void QmlProfilerFileWriter::setQmlEvents(const QVector<QmlProfilerDataModel::QmlEventTypeData> &types,
                                         const QVector<QmlProfilerDataModel::QmlEventData> &events)
{
    m_qmlEvents = types;
    m_ranges = events;
}

void QmlProfilerFileWriter::save(QIODevice *device)
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

     for (int typeIndex = 0; typeIndex < m_qmlEvents.size(); ++typeIndex) {
        const QmlProfilerDataModel::QmlEventTypeData &event = m_qmlEvents[typeIndex];

        stream.writeStartElement(_("event"));
        stream.writeAttribute(_("index"), QString::number(typeIndex));
        stream.writeTextElement(_("displayname"), event.displayName);
        stream.writeTextElement(_("type"), qmlTypeAsString(event.message, event.rangeType));
        if (!event.location.filename.isEmpty()) {
            stream.writeTextElement(_("filename"), event.location.filename);
            stream.writeTextElement(_("line"), QString::number(event.location.line));
            stream.writeTextElement(_("column"), QString::number(event.location.column));
        }

        if (!event.data.isEmpty())
            stream.writeTextElement(_("details"), event.data);

        if (event.rangeType == Binding)
            stream.writeTextElement(_("bindingType"), QString::number(event.detailType));
        if (event.message == Event && event.detailType == AnimationFrame)
            stream.writeTextElement(_("animationFrame"), QString::number(event.detailType));
        if (event.message == PixmapCacheEvent)
            stream.writeTextElement(_("cacheEventType"), QString::number(event.detailType));
        if (event.message == SceneGraphFrame)
            stream.writeTextElement(_("sgEventType"), QString::number(event.detailType));
        if (event.message == MemoryAllocation)
            stream.writeTextElement(_("memoryEventType"), QString::number(event.detailType));
        stream.writeEndElement();
    }
    stream.writeEndElement(); // eventData

    stream.writeStartElement(_("profilerDataModel"));

    for (int rangeIndex = 0; rangeIndex < m_ranges.size(); ++rangeIndex) {
        const QmlProfilerDataModel::QmlEventData &range = m_ranges[rangeIndex];

        stream.writeStartElement(_("range"));
        stream.writeAttribute(_("startTime"), QString::number(range.startTime));
        if (range.duration > 0) // no need to store duration of instantaneous events
            stream.writeAttribute(_("duration"), QString::number(range.duration));
        stream.writeAttribute(_("eventIndex"), QString::number(range.typeIndex));

        const QmlProfilerDataModel::QmlEventTypeData &event = m_qmlEvents[range.typeIndex];

        // special: animation event
        if (event.message == QmlDebug::Event && event.detailType == QmlDebug::AnimationFrame) {
            stream.writeAttribute(_("framerate"), QString::number(range.numericData1));
            stream.writeAttribute(_("animationcount"), QString::number(range.numericData2));
            stream.writeAttribute(_("thread"), QString::number(range.numericData3));
        }

        // special: pixmap cache event
        if (event.message == QmlDebug::PixmapCacheEvent) {
            if (event.detailType == PixmapSizeKnown) {
                stream.writeAttribute(_("width"), QString::number(range.numericData1));
                stream.writeAttribute(_("height"), QString::number(range.numericData2));
            }

            if (event.detailType == PixmapReferenceCountChanged ||
                    event.detailType == PixmapCacheCountChanged)
                stream.writeAttribute(_("refCount"), QString::number(range.numericData3));
        }

        if (event.message == QmlDebug::SceneGraphFrame) {
            // special: scenegraph frame events
            if (range.numericData1 > 0)
                stream.writeAttribute(_("timing1"), QString::number(range.numericData1));
            if (range.numericData2 > 0)
                stream.writeAttribute(_("timing2"), QString::number(range.numericData2));
            if (range.numericData3 > 0)
                stream.writeAttribute(_("timing3"), QString::number(range.numericData3));
            if (range.numericData4 > 0)
                stream.writeAttribute(_("timing4"), QString::number(range.numericData4));
            if (range.numericData5 > 0)
                stream.writeAttribute(_("timing5"), QString::number(range.numericData5));
        }

        // special: memory allocation event
        if (event.message == QmlDebug::MemoryAllocation)
            stream.writeAttribute(_("amount"), QString::number(range.numericData1));

        stream.writeEndElement();
    }
    stream.writeEndElement(); // profilerDataModel

    m_v8Model->save(stream);

    stream.writeEndElement(); // trace
    stream.writeEndDocument();
}


} // namespace Internal
} // namespace QmlProfiler
