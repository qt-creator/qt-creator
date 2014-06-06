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

#include "qmlprofilertimelinemodelproxy.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerdatamodel.h"
#include "sortedtimelinemodel.h"
#include "singlecategorytimelinemodel_p.h"

#include <QCoreApplication>
#include <QVector>
#include <QHash>
#include <QUrl>
#include <QString>
#include <QStack>

#include <QDebug>

namespace QmlProfiler {
namespace Internal {

class RangeTimelineModel::RangeTimelineModelPrivate :
        public SortedTimelineModel<RangeTimelineModel::QmlRangeEventStartInstance,
                                   SingleCategoryTimelineModel::SingleCategoryTimelineModelPrivate>
{
public:
    // convenience functions
    void computeNestingContracted();
    void computeExpandedLevels();
    void findBindingLoops();

    QVector <RangeTimelineModel::QmlRangeEventData> eventDict;
    QVector <QString> eventHashes;
    int expandedRows;
    int contractedRows;
    bool seenPaintEvent;
private:
    Q_DECLARE_PUBLIC(RangeTimelineModel)
};

RangeTimelineModel::RangeTimelineModel(QmlDebug::RangeType rangeType, QObject *parent)
    : SingleCategoryTimelineModel(new RangeTimelineModelPrivate,
                                  QLatin1String("RangeTimelineModel"), categoryLabel(rangeType),
                                  QmlDebug::MaximumMessage, rangeType, parent)
{
    Q_D(RangeTimelineModel);
    d->seenPaintEvent = false;
    d->expandedRows = 1;
    d->contractedRows = 1;
}

void RangeTimelineModel::clear()
{
    Q_D(RangeTimelineModel);
    d->clear();
    d->eventDict.clear();
    d->eventHashes.clear();
    d->expandedRows = 1;
    d->contractedRows = 1;
    d->seenPaintEvent = false;
    d->expanded = false;

    d->modelManager->modelProxyCountUpdated(d->modelId, 0, 1);
}

void RangeTimelineModel::loadData()
{
    Q_D(RangeTimelineModel);
    clear();
    QmlProfilerDataModel *simpleModel = d->modelManager->qmlModel();
    if (simpleModel->isEmpty())
        return;

    int lastEventId = 0;

    QMap<QString, int> eventIdsByHash;

    // collect events
    const QVector<QmlProfilerDataModel::QmlEventData> eventList = simpleModel->getEvents();
    foreach (const QmlProfilerDataModel::QmlEventData &event, eventList) {
        if (!eventAccepted(event))
            continue;
        if (event.rangeType == QmlDebug::Painting)
            d->seenPaintEvent = true;

        QString eventHash = QmlProfilerDataModel::getHashString(event);

        // store in dictionary
        int eventId;
        QMap<QString, int>::const_iterator i = eventIdsByHash.constFind(eventHash);
        if (i == eventIdsByHash.cend()) {
            QmlRangeEventData rangeEventData = {
                event.displayName,
                event.data.join(QLatin1String(" ")),
                event.location,
                lastEventId++ // event id
            };
            d->eventDict << rangeEventData;
            eventId = d->eventHashes.size();
            eventIdsByHash.insert(eventHash, eventId);
            d->eventHashes << eventHash;
        } else {
            eventId = i.value();
        }

        // store starttime-based instance
        d->insert(event.startTime, event.duration, QmlRangeEventStartInstance(eventId));

        d->modelManager->modelProxyCountUpdated(d->modelId, d->count(), eventList.count() * 6);
    }

    d->modelManager->modelProxyCountUpdated(d->modelId, 2, 6);

    // compute range nesting
    d->computeNesting();

    // compute nestingLevel - nonexpanded
    d->computeNestingContracted();

    d->modelManager->modelProxyCountUpdated(d->modelId, 3, 6);

    // compute nestingLevel - expanded
    d->computeExpandedLevels();

    d->modelManager->modelProxyCountUpdated(d->modelId, 4, 6);

    d->findBindingLoops();

    d->modelManager->modelProxyCountUpdated(d->modelId, 5, 6);

    d->modelManager->modelProxyCountUpdated(d->modelId, 1, 1);
}

void RangeTimelineModel::RangeTimelineModelPrivate::computeNestingContracted()
{
    Q_Q(RangeTimelineModel);
    int i;
    int eventCount = count();

    QList<int> nestingLevels;
    QList< QHash<int, qint64> > endtimesPerNestingLevel;

    for (i = 0; i < QmlDebug::MaximumRangeType; i++) {
        nestingLevels << QmlDebug::Constants::QML_MIN_LEVEL;
        QHash<int, qint64> dummyHash;
        dummyHash[QmlDebug::Constants::QML_MIN_LEVEL] = 0;
        endtimesPerNestingLevel << dummyHash;
    }

    for (i = 0; i < eventCount; i++) {
        qint64 st = ranges[i].start;
        int type = q->getEventType(i);

        // per type
        if (endtimesPerNestingLevel[type][nestingLevels[type]] > st) {
            nestingLevels[type]++;
        } else {
            while (nestingLevels[type] > QmlDebug::Constants::QML_MIN_LEVEL &&
                   endtimesPerNestingLevel[type][nestingLevels[type]-1] <= st)
                nestingLevels[type]--;
        }
        endtimesPerNestingLevel[type][nestingLevels[type]] =
                st + ranges[i].duration;

        ranges[i].displayRowCollapsed = nestingLevels[type];
    }

    // nestingdepth
    for (i = 0; i < eventCount; i++) {
        if (contractedRows <= ranges[i].displayRowCollapsed)
            contractedRows = ranges[i].displayRowCollapsed + 1;
    }
}

void RangeTimelineModel::RangeTimelineModelPrivate::computeExpandedLevels()
{
    QHash<int, int> eventRow;
    int eventCount = count();
    for (int i = 0; i < eventCount; i++) {
        int eventId = ranges[i].eventId;
        if (!eventRow.contains(eventId)) {
            eventRow[eventId] = expandedRows++;
        }
        ranges[i].displayRowExpanded = eventRow[eventId];
    }
}

void RangeTimelineModel::RangeTimelineModelPrivate::findBindingLoops()
{
    if (rangeType != QmlDebug::Binding && rangeType != QmlDebug::HandlingSignal)
        return;

    typedef QPair<QString, int> CallStackEntry;
    QStack<CallStackEntry> callStack;

    for (int i = 0; i < count(); ++i) {
        Range *event = &ranges[i];

        const QString eventHash = eventHashes.at(event->eventId);
        const Range *potentialParent = callStack.isEmpty()
                ? 0 : &ranges[callStack.top().second];

        while (potentialParent
               && !(potentialParent->start + potentialParent->duration > event->start)) {
            callStack.pop();
            potentialParent = callStack.isEmpty() ? 0
                                                  : &ranges[callStack.top().second];
        }

        // check whether event is already in stack
        for (int ii = 0; ii < callStack.size(); ++ii) {
            if (callStack.at(ii).first == eventHash) {
                event->bindingLoopHead = callStack.at(ii).second;
                break;
            }
        }


        CallStackEntry newEntry(eventHash, i);
        callStack.push(newEntry);
    }

}

/////////////////// QML interface

int RangeTimelineModel::rowCount() const
{
    Q_D(const RangeTimelineModel);
    // special for paint events: show only when empty model or there's actual events
    if (d->rangeType == QmlDebug::Painting && !d->seenPaintEvent)
        return 0;
    if (d->expanded)
        return d->expandedRows;
    else
        return d->contractedRows;
}

QString RangeTimelineModel::categoryLabel(int categoryIndex)
{
    switch (categoryIndex) {
    case 0: return QCoreApplication::translate("MainView", "Painting"); break;
    case 1: return QCoreApplication::translate("MainView", "Compiling"); break;
    case 2: return QCoreApplication::translate("MainView", "Creating"); break;
    case 3: return QCoreApplication::translate("MainView", "Binding"); break;
    case 4: return QCoreApplication::translate("MainView", "Handling Signal"); break;
    case 5: return QCoreApplication::translate("MainView", "JavaScript"); break;
    default: return QString();
    }
}

int RangeTimelineModel::getEventType(int index) const
{
    Q_D(const RangeTimelineModel);
    Q_UNUSED(index);
    return d->rangeType;
}

int RangeTimelineModel::getEventRow(int index) const
{
    Q_D(const RangeTimelineModel);
    if (d->expanded)
        return d->range(index).displayRowExpanded;
    else
        return d->range(index).displayRowCollapsed;
}

int RangeTimelineModel::getEventId(int index) const
{
    Q_D(const RangeTimelineModel);
    return d->range(index).eventId;
}

int RangeTimelineModel::getBindingLoopDest(int index) const
{
    Q_D(const RangeTimelineModel);
    return d->range(index).bindingLoopHead;
}

QColor RangeTimelineModel::getColor(int index) const
{
    return getEventColor(index);
}

const QVariantList RangeTimelineModel::getLabels() const
{
    Q_D(const RangeTimelineModel);
    QVariantList result;

    if (d->expanded) {
        int eventCount = d->eventDict.count();
        for (int i = 0; i < eventCount; i++) {
            QVariantMap element;
            element.insert(QLatin1String("displayName"), QVariant(d->eventDict[i].displayName));
            element.insert(QLatin1String("description"), QVariant(d->eventDict[i].details));
            element.insert(QLatin1String("id"), QVariant(d->eventDict[i].eventId));
            result << element;
        }
    }

    return result;
}

const QVariantList RangeTimelineModel::getEventDetails(int index) const
{
    Q_D(const RangeTimelineModel);
    QVariantList result;
    int eventId = getEventId(index);

    static const char trContext[] = "RangeDetails";
    {
        QVariantMap valuePair;
        valuePair.insert(QLatin1String("title"), QVariant(categoryLabel(d->rangeType)));
        result << valuePair;
    }

    // duration
    {
        QVariantMap valuePair;
        valuePair.insert(QCoreApplication::translate(trContext, "Duration:"),
                         QVariant(QmlProfilerBaseModel::formatTime(d->range(index).duration)));
        result << valuePair;
    }

    // details
    {
        QVariantMap valuePair;
        QString detailsString = d->eventDict[eventId].details;
        if (detailsString.length() > 40)
            detailsString = detailsString.left(40) + QLatin1String("...");
        valuePair.insert(QCoreApplication::translate(trContext, "Details:"), QVariant(detailsString));
        result << valuePair;
    }

    // location
    {
        QVariantMap valuePair;
        valuePair.insert(QCoreApplication::translate(trContext, "Location:"), QVariant(d->eventDict[eventId].displayName));
        result << valuePair;
    }

    // isbindingloop
    {}


    return result;
}

const QVariantMap RangeTimelineModel::getEventLocation(int index) const
{
    Q_D(const RangeTimelineModel);
    QVariantMap result;
    int eventId = getEventId(index);

    QmlDebug::QmlEventLocation location
            = d->eventDict.at(eventId).location;

    result.insert(QLatin1String("file"), location.filename);
    result.insert(QLatin1String("line"), location.line);
    result.insert(QLatin1String("column"), location.column);

    return result;
}

int RangeTimelineModel::getEventIdForHash(const QString &eventHash) const
{
    Q_D(const RangeTimelineModel);
    return d->eventHashes.indexOf(eventHash);
}

int RangeTimelineModel::getEventIdForLocation(const QString &filename, int line, int column) const
{
    Q_D(const RangeTimelineModel);
    // if this is called from v8 view, we don't have the column number, it will be -1
    foreach (const QmlRangeEventData &eventData, d->eventDict) {
        if (eventData.location.filename == filename &&
                eventData.location.line == line &&
                (column == -1 || eventData.location.column == column))
            return eventData.eventId;
    }
    return -1;
}



}
}
