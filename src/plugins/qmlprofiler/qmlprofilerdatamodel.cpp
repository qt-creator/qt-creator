/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlprofilerdatamodel.h"
#include "qmlprofilerbasemodel_p.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilernotesmodel.h"
#include <qmldebug/qmlprofilereventtypes.h>
#include <utils/qtcassert.h>
#include <QUrl>
#include <QDebug>
#include <algorithm>

namespace QmlProfiler {

class QmlProfilerDataModel::QmlProfilerDataModelPrivate :
        public QmlProfilerBaseModel::QmlProfilerBaseModelPrivate
{
public:
    QmlProfilerDataModelPrivate(QmlProfilerDataModel *qq) : QmlProfilerBaseModelPrivate(qq) {}
    QVector<QmlEventTypeData> eventTypes;
    QVector<QmlEventData> eventList;
    QVector<QmlEventNoteData> eventNotes;
    QHash<QmlEventTypeData, int> eventTypeIds;
private:
    Q_DECLARE_PUBLIC(QmlProfilerDataModel)
};

QString getDisplayName(const QmlProfilerDataModel::QmlEventTypeData &event)
{
    if (event.location.filename.isEmpty()) {
        return QmlProfilerDataModel::tr("<bytecode>");
    } else {
        const QString filePath = QUrl(event.location.filename).path();
        return filePath.mid(filePath.lastIndexOf(QLatin1Char('/')) + 1) + QLatin1Char(':') +
                QString::number(event.location.line);
    }
}

QString getInitialDetails(const QmlProfilerDataModel::QmlEventTypeData &event)
{
    QString details;
    // generate details string
    if (!event.data.isEmpty()) {
        details = event.data;
        details = details.replace(QLatin1Char('\n'),QLatin1Char(' ')).simplified();
        if (details.isEmpty()) {
            if (event.rangeType == QmlDebug::Javascript)
                details = QmlProfilerDataModel::tr("anonymous function");
        } else {
            QRegExp rewrite(QLatin1String("\\(function \\$(\\w+)\\(\\) \\{ (return |)(.+) \\}\\)"));
            bool match = rewrite.exactMatch(details);
            if (match)
                details = rewrite.cap(1) + QLatin1String(": ") + rewrite.cap(3);
            if (details.startsWith(QLatin1String("file://")) ||
                    details.startsWith(QLatin1String("qrc:/")))
                details = details.mid(details.lastIndexOf(QLatin1Char('/')) + 1);
        }
    } else if (event.rangeType == QmlDebug::Painting) {
        // QtQuick1 animations always run in GUI thread.
        details = QmlProfilerDataModel::tr("GUI Thread");
    }

    return details;
}


//////////////////////////////////////////////////////////////////////////////

QmlProfilerDataModel::QmlProfilerDataModel(Utils::FileInProjectFinder *fileFinder,
                                                     QmlProfilerModelManager *parent)
    : QmlProfilerBaseModel(fileFinder, parent, new QmlProfilerDataModelPrivate(this))
{
    Q_D(QmlProfilerDataModel);
    // The document loading is very expensive.
    d->modelManager->setProxyCountWeight(d->modelId, 4);
}

const QVector<QmlProfilerDataModel::QmlEventData> &QmlProfilerDataModel::getEvents() const
{
    Q_D(const QmlProfilerDataModel);
    return d->eventList;
}

const QVector<QmlProfilerDataModel::QmlEventTypeData> &QmlProfilerDataModel::getEventTypes() const
{
    Q_D(const QmlProfilerDataModel);
    return d->eventTypes;
}

const QVector<QmlProfilerDataModel::QmlEventNoteData> &QmlProfilerDataModel::getEventNotes() const
{
    Q_D(const QmlProfilerDataModel);
    return d->eventNotes;
}

void QmlProfilerDataModel::setData(qint64 traceStart, qint64 traceEnd,
                                   const QVector<QmlProfilerDataModel::QmlEventTypeData> &types,
                                   const QVector<QmlProfilerDataModel::QmlEventData> &events)
{
    Q_D(QmlProfilerDataModel);
    d->modelManager->traceTime()->setTime(traceStart, traceEnd);
    d->eventList = events;
    d->eventTypes = types;
    for (int id = 0; id < types.count(); ++id)
        d->eventTypeIds[types[id]] = id;
    // Half the work is done. complete() will do the rest.
    d->modelManager->modelProxyCountUpdated(d->modelId, 1, 2);
}

void QmlProfilerDataModel::setNoteData(const QVector<QmlProfilerDataModel::QmlEventNoteData> &notes)
{
    Q_D(QmlProfilerDataModel);
    d->eventNotes = notes;
}

int QmlProfilerDataModel::count() const
{
    Q_D(const QmlProfilerDataModel);
    return d->eventList.count();
}

void QmlProfilerDataModel::clear()
{
    Q_D(QmlProfilerDataModel);
    d->eventList.clear();
    d->eventTypes.clear();
    d->eventTypeIds.clear();
    d->eventNotes.clear();
    // This call emits changed(). Don't emit it again here.
    QmlProfilerBaseModel::clear();
}

bool QmlProfilerDataModel::isEmpty() const
{
    Q_D(const QmlProfilerDataModel);
    return d->eventList.isEmpty();
}

inline static bool operator<(const QmlProfilerDataModel::QmlEventData &t1,
                             const QmlProfilerDataModel::QmlEventData &t2)
{
    return t1.startTime < t2.startTime;
}

inline static uint qHash(const QmlProfilerDataModel::QmlEventTypeData &type)
{
    return qHash(type.location.filename) ^
            ((type.location.line & 0xfff) |             // 12 bits of line number
            ((type.message << 12) & 0xf000) |           // 4 bits of message
            ((type.location.column << 16) & 0xff0000) | // 8 bits of column
            ((type.rangeType << 24) & 0xf000000) |      // 4 bits of rangeType
            ((type.detailType << 28) & 0xf0000000));    // 4 bits of detailType
}

inline static bool operator==(const QmlProfilerDataModel::QmlEventTypeData &type1,
                              const QmlProfilerDataModel::QmlEventTypeData &type2)
{
    return type1.message == type2.message && type1.rangeType == type2.rangeType &&
            type1.detailType == type2.detailType && type1.location.line == type2.location.line &&
            type1.location.column == type2.location.column &&
            // compare filename last as it's expensive.
            type1.location.filename == type2.location.filename;
}

void QmlProfilerDataModel::complete()
{
    Q_D(QmlProfilerDataModel);
    // post-processing

    // sort events by start time, using above operator<
    std::sort(d->eventList.begin(), d->eventList.end());

    // rewrite strings
    int n = d->eventTypes.count();
    for (int i = 0; i < n; i++) {
        QmlEventTypeData *event = &d->eventTypes[i];
        event->displayName = getDisplayName(*event);
        event->data = getInitialDetails(*event);

        //
        // request further details from files
        //

        if (event->rangeType != QmlDebug::Binding && event->rangeType != QmlDebug::HandlingSignal)
            continue;

        // This skips anonymous bindings in Qt4.8 (we don't have valid location data for them)
        if (event->location.filename.isEmpty())
            continue;

        // Skip non-anonymous bindings from Qt4.8 (we already have correct details for them)
        if (event->location.column == -1)
            continue;

        d->detailsRewriter->requestDetailsForLocation(i, event->location);
        d->modelManager->modelProxyCountUpdated(d->modelId, i + n, n * 2);
    }

    // Allow changed() event only after documents have been reloaded to avoid
    // unnecessary updates of child models.
    QmlProfilerBaseModel::complete();
}

void QmlProfilerDataModel::addQmlEvent(QmlDebug::Message message, QmlDebug::RangeType rangeType,
                                            int detailType, qint64 startTime,
                                            qint64 duration, const QString &data,
                                            const QmlDebug::QmlEventLocation &location,
                                            qint64 ndata1, qint64 ndata2, qint64 ndata3,
                                            qint64 ndata4, qint64 ndata5)
{
    Q_D(QmlProfilerDataModel);
    QString displayName;

    QmlEventTypeData typeData = {displayName, location, message, rangeType, detailType, data};

    // Special case for QtQuick 1 Compiling and Creating events: filename is in the "data" field.
    if ((rangeType == QmlDebug::Compiling || rangeType == QmlDebug::Creating) &&
            location.filename.isEmpty()) {
        typeData.location.filename = data;
        typeData.location.line = typeData.location.column = 1;
    }

    QmlEventData eventData = {-1, startTime, duration, ndata1, ndata2, ndata3, ndata4, ndata5};

    QHash<QmlEventTypeData, int>::Iterator it = d->eventTypeIds.find(typeData);
    if (it != d->eventTypeIds.end()) {
        eventData.typeIndex = it.value();
    } else {
        eventData.typeIndex = d->eventTypes.size();
        d->eventTypeIds[typeData] = eventData.typeIndex;
        d->eventTypes.append(typeData);
    }

    d->eventList.append(eventData);

    d->modelManager->modelProxyCountUpdated(d->modelId, startTime,
                                            d->modelManager->traceTime()->duration() * 2);
}

qint64 QmlProfilerDataModel::lastTimeMark() const
{
    Q_D(const QmlProfilerDataModel);
    if (d->eventList.isEmpty())
        return 0;

    return d->eventList.last().startTime + d->eventList.last().duration;
}

void QmlProfilerDataModel::detailsChanged(int requestId, const QString &newString)
{
    Q_D(QmlProfilerDataModel);
    QTC_ASSERT(requestId < d->eventTypes.count(), return);

    QmlEventTypeData *event = &d->eventTypes[requestId];
    event->data = newString;
}

}
