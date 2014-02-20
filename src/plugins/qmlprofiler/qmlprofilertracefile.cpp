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

const char TYPE_PAINTING_STR[] = "Painting";
const char TYPE_COMPILING_STR[] = "Compiling";
const char TYPE_CREATING_STR[] = "Creating";
const char TYPE_BINDING_STR[] = "Binding";
const char TYPE_HANDLINGSIGNAL_STR[] = "HandlingSignal";
const char TYPE_PIXMAPCACHE_STR[] = "PixmapCache";
const char TYPE_SCENEGRAPH_STR[] = "SceneGraph";

#define _(X) QLatin1String(X)


//
// "be strict in your output but tolerant in your inputs"
//

namespace QmlProfiler {
namespace Internal {

static QmlEventType qmlEventTypeAsEnum(const QString &typeString)
{
    if (typeString == _(TYPE_PAINTING_STR)) {
        return Painting;
    } else if (typeString == _(TYPE_COMPILING_STR)) {
        return Compiling;
    } else if (typeString == _(TYPE_CREATING_STR)) {
        return Creating;
    } else if (typeString == _(TYPE_BINDING_STR)) {
        return Binding;
    } else if (typeString == _(TYPE_HANDLINGSIGNAL_STR)) {
        return HandlingSignal;
    } else if (typeString == _(TYPE_PIXMAPCACHE_STR)) {
        return PixmapCacheEvent;
    } else if (typeString == _(TYPE_SCENEGRAPH_STR)) {
        return SceneGraphFrameEvent;
    } else {
        bool isNumber = false;
        int type = typeString.toUInt(&isNumber);
        if (isNumber)
            return (QmlEventType)type;
        else
            return MaximumQmlEventType;
    }
}

static QString qmlEventTypeAsString(QmlEventType typeEnum)
{
    switch (typeEnum) {
    case Painting:
        return _(TYPE_PAINTING_STR);
        break;
    case Compiling:
        return _(TYPE_COMPILING_STR);
        break;
    case Creating:
        return _(TYPE_CREATING_STR);
        break;
    case Binding:
        return _(TYPE_BINDING_STR);
        break;
    case HandlingSignal:
        return _(TYPE_HANDLINGSIGNAL_STR);
        break;
    case PixmapCacheEvent:
        return _(TYPE_PIXMAPCACHE_STR);
        break;
    case SceneGraphFrameEvent:
        return _(TYPE_SCENEGRAPH_STR);
        break;
    default:
        return QString::number((int)typeEnum);
    }
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
         processQmlEvents();

         return true;
     }
}

void QmlProfilerFileReader::loadEventData(QXmlStreamReader &stream)
{
    QTC_ASSERT(stream.name() == _("eventData"), return);

    QXmlStreamAttributes attributes = stream.attributes();

    int eventIndex = -1;
    QmlEvent event = {
            QString(), // displayname
            QString(), // filename
            QString(), // details
            Painting, // type
            QmlBinding,  // bindingType, set for backwards compatibility
            0, // line
            0 // column
    };
    const QmlEvent defaultEvent = event;

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
                event.type = qmlEventTypeAsEnum(readData);
                break;
            }

            if (elementName == _("filename")) {
                event.filename = readData;
                break;
            }

            if (elementName == _("line")) {
                event.line = readData.toInt();
                break;
            }

            if (elementName == _("column")) {
                event.column = readData.toInt();
                break;
            }

            if (elementName == _("details")) {
                event.details = readData;
                break;
            }

            if (elementName == _("bindingType") ||
                    elementName == _("animationFrame") ||
                    elementName == _("cacheEventType") ||
                    elementName == _("sgEventType")) {
                event.bindingType = readData.toInt();
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
                Range range = { 0, 0, 0, 0, 0, 0, 0 };

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
                if (attributes.hasAttribute(_("width")))
                    range.numericData1 = attributes.value(_("width")).toString().toLongLong();
                if (attributes.hasAttribute(_("height")))
                    range.numericData2 = attributes.value(_("height")).toString().toLongLong();
                if (attributes.hasAttribute(_("refCount")))
                    range.numericData3 = attributes.value(_("refCount")).toString().toLongLong();
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


                int eventIndex = attributes.value(_("eventIndex")).toString().toInt();


                m_ranges.append(QPair<Range,int>(range, eventIndex));
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

void QmlProfilerFileReader::processQmlEvents()
{
    for (int i = 0; i < m_ranges.size(); ++i) {
        Range range = m_ranges[i].first;
        int eventIndex = m_ranges[i].second;

        if (eventIndex < 0 || eventIndex >= m_qmlEvents.size()) {
            qWarning() << ".qtd file - range index" << eventIndex
                       << "is outside of bounds (0, " << m_qmlEvents.size() << ")";
            continue;
        }

        QmlEvent &event = m_qmlEvents[eventIndex];

        emit rangedEvent(event.type, event.bindingType, range.startTime, range.duration,
                         QStringList(event.details),
                         QmlEventLocation(event.filename, event.line, event.column),
                         range.numericData1,range.numericData2, range.numericData3, range.numericData4, range.numericData5);

    }
}

QmlProfilerFileWriter::QmlProfilerFileWriter(QObject *parent) :
    QObject(parent),
    m_startTime(0),
    m_endTime(0),
    m_measuredTime(0),
    m_v8Model(0)
{
    m_acceptedTypes << QmlDebug::Compiling << QmlDebug::Creating << QmlDebug::Binding << QmlDebug::HandlingSignal;
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

void QmlProfilerFileWriter::setQmlEvents(const QVector<QmlProfilerDataModel::QmlEventData> &events)
{
    foreach (const QmlProfilerDataModel::QmlEventData &event, events) {
        const QString hashStr = QmlProfilerDataModel::getHashString(event);
        if (!m_qmlEvents.contains(hashStr)) {
            QmlEvent e = { event.displayName,
                           event.location.filename,
                           event.data.join(_("")),
                           static_cast<QmlDebug::QmlEventType>(event.eventType),
                           event.bindingType,
                           event.location.line,
                           event.location.column
                         };
            m_qmlEvents.insert(hashStr, e);
        }

        Range r = { event.startTime, event.duration, event.numericData1, event.numericData2, event.numericData3, event.numericData4, event.numericData5 };
        m_ranges.append(QPair<Range, QString>(r, hashStr));
    }

    calculateMeasuredTime(events);
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

    QHash<QString,QmlEvent>::const_iterator eventIter = m_qmlEvents.constBegin();
    for (; eventIter != m_qmlEvents.constEnd(); ++eventIter) {
        QmlEvent event = eventIter.value();

        stream.writeStartElement(_("event"));
        stream.writeAttribute(_("index"), QString::number(m_qmlEvents.keys().indexOf(eventIter.key())));
        stream.writeTextElement(_("displayname"), event.displayName);
        stream.writeTextElement(_("type"), qmlEventTypeAsString(event.type));
        if (!event.filename.isEmpty()) {
            stream.writeTextElement(_("filename"), event.filename);
            stream.writeTextElement(_("line"), QString::number(event.line));
            stream.writeTextElement(_("column"), QString::number(event.column));
        }
        stream.writeTextElement(_("details"), event.details);
        if (event.type == Binding)
            stream.writeTextElement(_("bindingType"), QString::number(event.bindingType));
        if (event.type == Painting && event.bindingType == AnimationFrame)
            stream.writeTextElement(_("animationFrame"), QString::number(event.bindingType));
        if (event.type == PixmapCacheEvent)
            stream.writeTextElement(_("cacheEventType"), QString::number(event.bindingType));
        if (event.type == SceneGraphFrameEvent)
            stream.writeTextElement(_("sgEventType"), QString::number(event.bindingType));
        stream.writeEndElement();
    }
    stream.writeEndElement(); // eventData

    stream.writeStartElement(_("profilerDataModel"));

    QVector<QPair<Range, QString> >::const_iterator rangeIter = m_ranges.constBegin();
    for (; rangeIter != m_ranges.constEnd(); ++rangeIter) {
        Range range = rangeIter->first;
        QString eventHash = rangeIter->second;

        stream.writeStartElement(_("range"));
        stream.writeAttribute(_("startTime"), QString::number(range.startTime));
        if (range.duration > 0) // no need to store duration of instantaneous events
            stream.writeAttribute(_("duration"), QString::number(range.duration));
        stream.writeAttribute(_("eventIndex"), QString::number(m_qmlEvents.keys().indexOf(eventHash)));

        QmlEvent event = m_qmlEvents.value(eventHash);

        // special: animation event
        if (event.type == QmlDebug::Painting && event.bindingType == QmlDebug::AnimationFrame) {

            stream.writeAttribute(_("framerate"), QString::number(range.numericData1));
            stream.writeAttribute(_("animationcount"), QString::number(range.numericData2));
        }

        // special: pixmap cache event
        if (event.type == QmlDebug::PixmapCacheEvent) {
            // pixmap image size
            if (event.bindingType == 0) {
                stream.writeAttribute(_("width"), QString::number(range.numericData1));
                stream.writeAttribute(_("height"), QString::number(range.numericData2));
            }

            // reference count (1) / cache size changed (2)
            if (event.bindingType == 1 || event.bindingType == 2)
                stream.writeAttribute(_("refCount"), QString::number(range.numericData3));
        }

        if (event.type == QmlDebug::SceneGraphFrameEvent) {
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
        stream.writeEndElement();
    }
    stream.writeEndElement(); // profilerDataModel

    m_v8Model->save(stream);

    stream.writeEndElement(); // trace
    stream.writeEndDocument();
}

void QmlProfilerFileWriter::calculateMeasuredTime(const QVector<QmlProfilerDataModel::QmlEventData> &events)
{
    // measured time isn't used, but old clients might still need it
    // -> we calculate it explicitly

    qint64 duration = 0;

    QHash<int, qint64> endtimesPerLevel;
    int level = QmlDebug::Constants::QML_MIN_LEVEL;
    endtimesPerLevel[0] = 0;

    foreach (const QmlProfilerDataModel::QmlEventData &event, events) {
        // whitelist
        if (!m_acceptedTypes.contains(event.eventType))
            continue;

        // level computation
        if (endtimesPerLevel[level] > event.startTime) {
            level++;
        } else {
            while (level > QmlDebug::Constants::QML_MIN_LEVEL && endtimesPerLevel[level-1] <= event.startTime)
                level--;
        }
        endtimesPerLevel[level] = event.startTime + event.duration;
        if (level == QmlDebug::Constants::QML_MIN_LEVEL)
            duration += event.duration;
    }

    m_measuredTime = duration;
}


} // namespace Internal
} // namespace QmlProfiler
