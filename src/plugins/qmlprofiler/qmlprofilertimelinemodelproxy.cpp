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

#include <QCoreApplication>
#include <QVector>
#include <QHash>
#include <QUrl>
#include <QString>
#include <QStack>

#include <QDebug>

namespace QmlProfiler {
namespace Internal {

struct CategorySpan {
    bool expanded;
    int expandedRows;
    int contractedRows;
    int rowStart;
};

class BasicTimelineModel::BasicTimelineModelPrivate : public SortedTimelineModel<BasicTimelineModel::QmlRangeEventStartInstance>
{
public:
    // convenience functions
    void prepare();
    void computeNestingContracted();
    void computeExpandedLevels();
    void findBindingLoops();
    void computeRowStarts();

    QVector <BasicTimelineModel::QmlRangeEventData> eventDict;
    QVector <QString> eventHashes;
    QVector <CategorySpan> categorySpan;
    bool seenPaintEvent;
private:
    Q_DECLARE_PUBLIC(BasicTimelineModel)
};

BasicTimelineModel::BasicTimelineModel(QObject *parent)
    : AbstractTimelineModel(new BasicTimelineModelPrivate, QLatin1String("BasicTimelineModel"),
                            parent)
{
    Q_D(BasicTimelineModel);
    d->seenPaintEvent = false;
}

void BasicTimelineModel::clear()
{
    Q_D(BasicTimelineModel);
    d->SortedTimelineModel::clear();
    d->eventDict.clear();
    d->eventHashes.clear();
    d->categorySpan.clear();
    d->seenPaintEvent = false;

    d->modelManager->modelProxyCountUpdated(d->modelId, 0, 1);
}

void BasicTimelineModel::BasicTimelineModelPrivate::prepare()
{
    categorySpan.clear();
    for (int i = 0; i < QmlDebug::MaximumQmlEventType; i++) {
        CategorySpan newCategory = {false, 1, 1, i};
        categorySpan << newCategory;
    }
}

bool BasicTimelineModel::eventAccepted(const QmlProfilerDataModel::QmlEventData &event) const
{
    // only accept Qt4.x Painting events
    if (event.eventType == QmlDebug::Painting)
        return (event.bindingType == QmlDebug::QPainterEvent);

    return (event.eventType <= QmlDebug::Javascript);
}

void BasicTimelineModel::loadData()
{
    Q_D(BasicTimelineModel);
    clear();
    QmlProfilerDataModel *simpleModel = d->modelManager->qmlModel();
    if (simpleModel->isEmpty())
        return;

    int lastEventId = 0;

    d->prepare();

    QMap<QString, int> eventIdsByHash;

    // collect events
    const QVector<QmlProfilerDataModel::QmlEventData> eventList = simpleModel->getEvents();
    foreach (const QmlProfilerDataModel::QmlEventData &event, eventList) {
        if (!eventAccepted(event))
            continue;
        if (event.eventType == QmlDebug::Painting)
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
                (QmlDebug::QmlEventType)event.eventType,
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

    d->computeRowStarts();

    d->modelManager->modelProxyCountUpdated(d->modelId, 1, 1);
}

void BasicTimelineModel::BasicTimelineModelPrivate::computeNestingContracted()
{
    Q_Q(BasicTimelineModel);
    int i;
    int eventCount = count();

    QList<int> nestingLevels;
    QList< QHash<int, qint64> > endtimesPerNestingLevel;

    for (i = 0; i < QmlDebug::MaximumQmlEventType; i++) {
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
        int eventType = q->getEventType(i);
        if (categorySpan[eventType].contractedRows <= ranges[i].displayRowCollapsed)
            categorySpan[eventType].contractedRows = ranges[i].displayRowCollapsed + 1;
    }
}

void BasicTimelineModel::BasicTimelineModelPrivate::computeExpandedLevels()
{
    QHash<int, int> eventRow;
    int eventCount = count();
    for (int i = 0; i < eventCount; i++) {
        int eventId = ranges[i].eventId;
        int eventType = eventDict[eventId].eventType;
        if (!eventRow.contains(eventId)) {
            eventRow[eventId] = categorySpan[eventType].expandedRows++;
        }
        ranges[i].displayRowExpanded = eventRow[eventId];
    }
}

void BasicTimelineModel::BasicTimelineModelPrivate::findBindingLoops()
{
    typedef QPair<QString, int> CallStackEntry;
    QStack<CallStackEntry> callStack;

    for (int i = 0; i < count(); ++i) {
        Range *event = &ranges[i];

        BasicTimelineModel::QmlRangeEventData data = eventDict.at(event->eventId);

        static QVector<QmlDebug::QmlEventType> acceptedTypes =
                QVector<QmlDebug::QmlEventType>() << QmlDebug::Binding << QmlDebug::HandlingSignal;

        if (!acceptedTypes.contains(data.eventType))
            continue;

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

void BasicTimelineModel::BasicTimelineModelPrivate::computeRowStarts()
{
    Q_Q(BasicTimelineModel);
    int rowStart = 0;
    for (int i = 0; i < categorySpan.count(); i++) {
        categorySpan[i].rowStart = rowStart;
        rowStart += q->categoryDepth(i);
    }
}

/////////////////// QML interface

bool BasicTimelineModel::expanded(int category) const
{
    Q_D(const BasicTimelineModel);
    if (d->categorySpan.count() <= category)
        return false;
    return d->categorySpan[category].expanded;
}

void BasicTimelineModel::setExpanded(int category, bool expanded)
{
    Q_D(BasicTimelineModel);
    if (d->categorySpan.count() <= category)
        return;

    d->categorySpan[category].expanded = expanded;
    d->computeRowStarts();
    emit expandedChanged();
}

int BasicTimelineModel::categoryDepth(int categoryIndex) const
{
    Q_D(const BasicTimelineModel);
    // special for paint events: show only when empty model or there's actual events
    if (categoryIndex == QmlDebug::Painting && !d->seenPaintEvent)
        return 0;
    if (d->categorySpan.count() <= categoryIndex)
        return 1;
    if (d->categorySpan[categoryIndex].expanded)
        return d->categorySpan[categoryIndex].expandedRows;
    else
        return d->categorySpan[categoryIndex].contractedRows;
}

int BasicTimelineModel::categoryCount() const
{
    return 6;
}

const QString BasicTimelineModel::categoryLabel(int categoryIndex) const
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

int BasicTimelineModel::getEventType(int index) const
{
    Q_D(const BasicTimelineModel);
    return d->eventDict[d->range(index).eventId].eventType;
}

int BasicTimelineModel::getEventCategory(int index) const
{
    Q_D(const BasicTimelineModel);
    int evTy = getEventType(index);
    // special: paint events shown?
    if (!d->seenPaintEvent)
        return evTy - 1;
    return evTy;
}

int BasicTimelineModel::getEventRow(int index) const
{
    Q_D(const BasicTimelineModel);
    if (d->categorySpan[getEventType(index)].expanded)
        return d->range(index).displayRowExpanded + d->categorySpan[getEventType(index)].rowStart;
    else
        return d->range(index).displayRowCollapsed  + d->categorySpan[getEventType(index)].rowStart;
}

int BasicTimelineModel::getEventId(int index) const
{
    Q_D(const BasicTimelineModel);
    return d->range(index).eventId;
}

int BasicTimelineModel::getBindingLoopDest(int index) const
{
    Q_D(const BasicTimelineModel);
    return d->range(index).bindingLoopHead;
}

QColor BasicTimelineModel::getColor(int index) const
{
    return getEventColor(index);
}

const QVariantList BasicTimelineModel::getLabelsForCategory(int category) const
{
    Q_D(const BasicTimelineModel);
    QVariantList result;

    if (d->categorySpan.count() > category && d->categorySpan[category].expanded) {
        int eventCount = d->eventDict.count();
        for (int i = 0; i < eventCount; i++) {
            if (d->eventDict[i].eventType == category) {
                QVariantMap element;
                element.insert(QLatin1String("displayName"), QVariant(d->eventDict[i].displayName));
                element.insert(QLatin1String("description"), QVariant(d->eventDict[i].details));
                element.insert(QLatin1String("id"), QVariant(d->eventDict[i].eventId));
                result << element;
            }
        }
    }

    return result;
}

const QVariantList BasicTimelineModel::getEventDetails(int index) const
{
    Q_D(const BasicTimelineModel);
    QVariantList result;
    int eventId = getEventId(index);

    static const char trContext[] = "RangeDetails";
    {
        QVariantMap valuePair;
        valuePair.insert(QLatin1String("title"), QVariant(categoryLabel(d->eventDict[eventId].eventType)));
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

const QVariantMap BasicTimelineModel::getEventLocation(int index) const
{
    Q_D(const BasicTimelineModel);
    QVariantMap result;
    int eventId = getEventId(index);

    QmlDebug::QmlEventLocation location
            = d->eventDict.at(eventId).location;

    result.insert(QLatin1String("file"), location.filename);
    result.insert(QLatin1String("line"), location.line);
    result.insert(QLatin1String("column"), location.column);

    return result;
}

int BasicTimelineModel::getEventIdForHash(const QString &eventHash) const
{
    Q_D(const BasicTimelineModel);
    return d->eventHashes.indexOf(eventHash);
}

int BasicTimelineModel::getEventIdForLocation(const QString &filename, int line, int column) const
{
    Q_D(const BasicTimelineModel);
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
