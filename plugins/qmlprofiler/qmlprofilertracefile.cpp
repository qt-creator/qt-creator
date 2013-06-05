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
         QString elementName = stream.name().toString();
         switch (token) {
         case QXmlStreamReader::StartDocument :  continue;
         case QXmlStreamReader::StartElement : {
             if (elementName == _("trace")) {
                 QXmlStreamAttributes attributes = stream.attributes();
                 if (attributes.hasAttribute(_("version")))
                     validVersion = attributes.value(_("version")).toString()
                             == _(PROFILER_FILE_VERSION);
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
    QTC_ASSERT(stream.name().toString() == _("eventData"), return);

    QXmlStreamAttributes attributes = stream.attributes();
    if (attributes.hasAttribute(_("totalTime"))) {
        // not used any more
    }

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
        const QString elementName = stream.name().toString();

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

            if (elementName == _("bindingType")) {
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
    QTC_ASSERT(stream.name().toString() == _("profilerDataModel"), return);

    while (!stream.atEnd() && !stream.hasError()) {
        QXmlStreamReader::TokenType token = stream.readNext();
        const QString elementName = stream.name().toString();

        switch (token) {
        case QXmlStreamReader::StartElement: {
            if (elementName == _("range")) {
                Range range = { 0, 0 };

                const QXmlStreamAttributes attributes = stream.attributes();
                if (!attributes.hasAttribute(_("startTime"))
                        || !attributes.hasAttribute(_("duration"))
                        || !attributes.hasAttribute(_("eventIndex"))) {
                    // ignore incomplete entry
                    continue;
                }

                range.startTime = attributes.value(_("startTime")).toString().toLongLong();
                range.duration = attributes.value(_("duration")).toString().toLongLong();
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
                         QStringList(event.displayName), QmlEventLocation(event.filename,
                                                                          event.line,
                                                                          event.column));
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

void QmlProfilerFileWriter::setQmlEvents(const QVector<QmlProfilerSimpleModel::QmlEventData> &events)
{
    foreach (const QmlProfilerSimpleModel::QmlEventData &event, events) {
        const QString hashStr = QmlProfilerSimpleModel::getHashString(event);
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

        Range r = { event.startTime, event.duration,  };
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
            stream.writeTextElement(_("bindingType"), QString::number((int)event.bindingType));
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
        stream.writeAttribute(_("duration"), QString::number(range.duration));
        stream.writeAttribute(_("eventIndex"), QString::number(m_qmlEvents.keys().indexOf(eventHash)));

        QmlEvent event = m_qmlEvents.value(eventHash);
//        if (event.type == QmlDebug::Painting && range.animationCount >= 0) {
//            // animation frame
//            stream.writeAttribute(_("framerate"), QString::number(rangedEvent.frameRate));
//            stream.writeAttribute(_("animationcount"), QString::number(rangedEvent.animationCount));
//        }
        stream.writeEndElement();
    }
    stream.writeEndElement(); // profilerDataModel

    m_v8Model->save(stream);

    stream.writeEndElement(); // trace
    stream.writeEndDocument();
}

void QmlProfilerFileWriter::calculateMeasuredTime(const QVector<QmlProfilerSimpleModel::QmlEventData> &events)
{
    // measured time isn't used, but old clients might still need it
    // -> we calculate it explicitly

    qint64 duration = 0;

    QHash<int, qint64> endtimesPerLevel;
    int level = QmlDebug::Constants::QML_MIN_LEVEL;
    endtimesPerLevel[0] = 0;

    foreach (const QmlProfilerSimpleModel::QmlEventData &event, events) {
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
        if (level == QmlDebug::Constants::QML_MIN_LEVEL) {
            duration += event.duration;
        }
    }

    m_measuredTime = duration;
}


} // namespace Internal
} // namespace QmlProfiler
