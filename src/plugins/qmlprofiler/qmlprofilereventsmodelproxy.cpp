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

#include "qmlprofilereventsmodelproxy.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerdatamodel.h"

#include <utils/qtcassert.h>

#include <QVector>
#include <QHash>
#include <QUrl>
#include <QString>
#include <QStack>
#include <QElapsedTimer>

#include <QDebug>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerEventsModelProxy::QmlProfilerEventsModelProxyPrivate
{
public:
    QmlProfilerEventsModelProxyPrivate(QmlProfilerEventsModelProxy *qq) : q(qq) {}
    ~QmlProfilerEventsModelProxyPrivate() {}

    QHash<QString, QmlProfilerEventsModelProxy::QmlEventStats> data;

    QmlProfilerModelManager *modelManager;
    QmlProfilerEventsModelProxy *q;

    int modelId;

    QList<QmlDebug::RangeType> acceptedTypes;
    QSet<QString> eventsInBindingLoop;
};

QmlProfilerEventsModelProxy::QmlProfilerEventsModelProxy(QmlProfilerModelManager *modelManager, QObject *parent)
    : QObject(parent), d(new QmlProfilerEventsModelProxyPrivate(this))
{
    d->modelManager = modelManager;
    connect(modelManager->qmlModel(), SIGNAL(changed()), this, SLOT(dataChanged()));
    d->modelId = modelManager->registerModelProxy();

    // We're iterating twice in loadData.
    modelManager->setProxyCountWeight(d->modelId, 2);

    d->acceptedTypes << QmlDebug::Compiling << QmlDebug::Creating << QmlDebug::Binding << QmlDebug::HandlingSignal << QmlDebug::Javascript;
}

QmlProfilerEventsModelProxy::~QmlProfilerEventsModelProxy()
{
    delete d;
}

void QmlProfilerEventsModelProxy::setEventTypeAccepted(QmlDebug::RangeType type, bool accepted)
{
    if (accepted && !d->acceptedTypes.contains(type))
        d->acceptedTypes << type;
    else if (!accepted && d->acceptedTypes.contains(type))
        d->acceptedTypes.removeOne(type);
}

bool QmlProfilerEventsModelProxy::eventTypeAccepted(QmlDebug::RangeType type) const
{
    return d->acceptedTypes.contains(type);
}

const QList<QmlProfilerEventsModelProxy::QmlEventStats> QmlProfilerEventsModelProxy::getData() const
{
    return d->data.values();
}

void QmlProfilerEventsModelProxy::clear()
{
    d->modelManager->modelProxyCountUpdated(d->modelId, 0, 1);
    d->data.clear();
    d->eventsInBindingLoop.clear();
}

void QmlProfilerEventsModelProxy::limitToRange(qint64 rangeStart, qint64 rangeEnd)
{
    loadData(rangeStart, rangeEnd);
}

void QmlProfilerEventsModelProxy::dataChanged()
{
    if (d->modelManager->state() == QmlProfilerDataState::ProcessingData)
        loadData();
    else if (d->modelManager->state() == QmlProfilerDataState::ClearingData)
        clear();
}

QSet<QString> QmlProfilerEventsModelProxy::eventsInBindingLoop() const
{
    return d->eventsInBindingLoop;
}

void QmlProfilerEventsModelProxy::loadData(qint64 rangeStart, qint64 rangeEnd)
{
    clear();

    qint64 qmlTime = 0;
    qint64 lastEndTime = 0;
    QHash <QString, QVector<qint64> > durations;

    const bool checkRanges = (rangeStart != -1) && (rangeEnd != -1);

    const QVector<QmlProfilerDataModel::QmlEventData> eventList
            = d->modelManager->qmlModel()->getEvents();

    // used by binding loop detection
    typedef QPair<QString, const QmlProfilerDataModel::QmlEventData*> CallStackEntry;
    QStack<CallStackEntry> callStack;
    callStack.push(CallStackEntry(QString(), 0)); // artificial root

    for (int i = 0; i < eventList.size(); ++i) {
        const QmlProfilerDataModel::QmlEventData *event = &eventList[i];

        if (!d->acceptedTypes.contains(event->rangeType))
            continue;

        if (checkRanges) {
            if ((event->startTime + event->duration < rangeStart)
                    || (event->startTime > rangeEnd))
                continue;
        }

        // put event in hash
        QString hash = QmlProfilerDataModel::getHashString(*event);
        if (!d->data.contains(hash)) {
            QmlEventStats stats = {
                event->displayName,
                hash,
                event->data.join(QLatin1String(" ")),
                event->location,
                event->message,
                event->rangeType,
                event->detailType,
                event->duration,
                1, //calls
                event->duration, //minTime
                event->duration, // maxTime
                0, //timePerCall
                0, //percentOfTime
                0, //medianTime
                false //isBindingLoop
            };

            d->data.insert(hash, stats);

            // for median computing
            durations.insert(hash, QVector<qint64>());
            durations[hash].append(event->duration);
        } else {
            // update stats
            QmlEventStats *stats = &d->data[hash];

            stats->duration += event->duration;
            if (event->duration < stats->minTime)
                stats->minTime = event->duration;
            if (event->duration > stats->maxTime)
                stats->maxTime = event->duration;
            stats->calls++;

            // for median computing
            durations[hash].append(event->duration);
        }

        // qml time computation
        if (event->startTime > lastEndTime) { // assume parent event if starts before last end
            qmlTime += event->duration;
            lastEndTime = event->startTime + event->duration;
        }


        //
        // binding loop detection
        //
        const QmlProfilerDataModel::QmlEventData *potentialParent = callStack.top().second;
        while (potentialParent
               && !(potentialParent->startTime + potentialParent->duration > event->startTime)) {
            callStack.pop();
            potentialParent = callStack.top().second;
        }

        // check whether event is already in stack
        bool inLoop = false;
        for (int ii = 1; ii < callStack.size(); ++ii) {
            if (callStack.at(ii).first == hash)
                inLoop = true;
            if (inLoop)
                d->eventsInBindingLoop.insert(hash);
        }


        CallStackEntry newEntry(hash, event);
        callStack.push(newEntry);

        d->modelManager->modelProxyCountUpdated(d->modelId, i, eventList.count()*2);
    }

    // post-process: calc mean time, median time, percentoftime
    int i = d->data.size();
    int total = i * 2;
    foreach (const QString &hash, d->data.keys()) {
        QmlEventStats* stats = &d->data[hash];
        if (stats->calls > 0)
            stats->timePerCall = stats->duration / (double)stats->calls;

        QVector<qint64> eventDurations = durations.value(hash);
        if (!eventDurations.isEmpty()) {
            qSort(eventDurations);
            stats->medianTime = eventDurations.at(eventDurations.count()/2);
        }

        stats->percentOfTime = stats->duration * 100.0 / qmlTime;
        d->modelManager->modelProxyCountUpdated(d->modelId, i++, total);
    }

    // set binding loop flag
    foreach (const QString &eventHash, d->eventsInBindingLoop)
        d->data[eventHash].isBindingLoop = true;

    QString rootEventName = tr("<program>");
    QmlDebug::QmlEventLocation rootEventLocation(rootEventName, 1, 1);

    // insert root event
    QmlEventStats rootEvent = {
        rootEventName, //event.displayName,
        rootEventName, // hash
        tr("Main Program"), //event.details,
        rootEventLocation, // location
        QmlDebug::MaximumMessage,
        QmlDebug::Binding, // event type
        0, // binding type
        qmlTime + 1,
        1, //calls
        qmlTime + 1, //minTime
        qmlTime + 1, // maxTime
        qmlTime + 1, //timePerCall
        100.0, //percentOfTime
        qmlTime + 1, //medianTime;
        false
    };

    d->data.insert(rootEventName, rootEvent);

    d->modelManager->modelProxyCountUpdated(d->modelId, 1, 1);
    emit dataAvailable();
}

int QmlProfilerEventsModelProxy::count() const
{
    return d->data.count();
}

//////////////////////////////////////////////////////////////////////////////////
QmlProfilerEventRelativesModelProxy::QmlProfilerEventRelativesModelProxy(QmlProfilerModelManager *modelManager,
                                                                         QmlProfilerEventsModelProxy *eventsModel,
                                                                         QObject *parent)
    : QObject(parent)
{
    QTC_CHECK(modelManager);
    m_modelManager = modelManager;

    QTC_CHECK(eventsModel);
    m_eventsModel = eventsModel;

    // Load the child models whenever the parent model is done to get the filtering for JS/QML
    // right.
    connect(m_eventsModel, SIGNAL(dataAvailable()), this, SLOT(dataChanged()));
}

QmlProfilerEventRelativesModelProxy::~QmlProfilerEventRelativesModelProxy()
{
}

const QmlProfilerEventRelativesModelProxy::QmlEventRelativesMap QmlProfilerEventRelativesModelProxy::getData(const QString &hash) const
{
    if (m_data.contains(hash))
        return m_data[hash];
    return QmlEventRelativesMap();
}

int QmlProfilerEventRelativesModelProxy::count() const
{
    return m_data.count();
}

void QmlProfilerEventRelativesModelProxy::clear()
{
    m_data.clear();
}

void QmlProfilerEventRelativesModelProxy::dataChanged()
{
    loadData();

    emit dataAvailable();
}


//////////////////////////////////////////////////////////////////////////////////
QmlProfilerEventParentsModelProxy::QmlProfilerEventParentsModelProxy(QmlProfilerModelManager *modelManager,
                                                                     QmlProfilerEventsModelProxy *eventsModel,
                                                                     QObject *parent)
    : QmlProfilerEventRelativesModelProxy(modelManager, eventsModel, parent)
{}

QmlProfilerEventParentsModelProxy::~QmlProfilerEventParentsModelProxy()
{}

void QmlProfilerEventParentsModelProxy::loadData()
{
    clear();
    QmlProfilerDataModel *simpleModel = m_modelManager->qmlModel();
    if (simpleModel->isEmpty())
        return;

    QHash<QString, QmlProfilerDataModel::QmlEventData> cachedEvents;
    QString rootEventName = tr("<program>");
    QmlProfilerDataModel::QmlEventData rootEvent = {
        rootEventName,
        QmlDebug::MaximumMessage,
        QmlDebug::Binding,
        0,
        0,
        0,
        QStringList() << tr("Main Program"),
        QmlDebug::QmlEventLocation(rootEventName, 0, 0),
        0,0,0,0,0 // numericData fields
    };
    cachedEvents.insert(rootEventName, rootEvent);

    // for level computation
    QHash<int, qint64> endtimesPerLevel;
    int level = QmlDebug::Constants::QML_MIN_LEVEL;
    endtimesPerLevel[0] = 0;

    const QSet<QString> eventsInBindingLoop = m_eventsModel->eventsInBindingLoop();

    // compute parent-child relationship and call count
    QHash<int, QString> lastParent;
    //for (int index = fromIndex; index <= toIndex; index++) {
    const QVector<QmlProfilerDataModel::QmlEventData> eventList = simpleModel->getEvents();
    foreach (const QmlProfilerDataModel::QmlEventData &event, eventList) {
        // whitelist
        if (!m_eventsModel->eventTypeAccepted(event.rangeType))
            continue;

        // level computation
        if (endtimesPerLevel[level] > event.startTime) {
            level++;
        } else {
            while (level > QmlDebug::Constants::QML_MIN_LEVEL && endtimesPerLevel[level-1] <= event.startTime)
                level--;
        }
        endtimesPerLevel[level] = event.startTime + event.duration;


        QString parentHash = rootEventName;
        QString eventHash = QmlProfilerDataModel::getHashString(event);

        // save in cache
        if (!cachedEvents.contains(eventHash))
            cachedEvents.insert(eventHash, event);

        if (level > QmlDebug::Constants::QML_MIN_LEVEL && lastParent.contains(level-1))
            parentHash = lastParent[level-1];

        QmlProfilerDataModel::QmlEventData *parentEvent = &(cachedEvents[parentHash]);

        // generate placeholder if needed
        if (!m_data.contains(eventHash))
            m_data.insert(eventHash, QmlEventRelativesMap());

        if (m_data[eventHash].contains(parentHash)) {
            QmlEventRelativesData *parent = &(m_data[eventHash][parentHash]);
            parent->calls++;
            parent->duration += event.duration;
        } else {
            m_data[eventHash].insert(parentHash, QmlEventRelativesData());
            QmlEventRelativesData *parent = &(m_data[eventHash][parentHash]);
            parent->displayName = parentEvent->displayName;
            parent->rangeType = parentEvent->rangeType;
            parent->duration = event.duration;
            parent->calls = 1;
            parent->details = parentEvent->data.join(QLatin1String(""));
            parent->isBindingLoop = eventsInBindingLoop.contains(parentHash);
        }

        // now lastparent is a string with the hash
        lastParent[level] = eventHash;
    }
}

//////////////////////////////////////////////////////////////////////////////////
QmlProfilerEventChildrenModelProxy::QmlProfilerEventChildrenModelProxy(QmlProfilerModelManager *modelManager,
                                                                       QmlProfilerEventsModelProxy *eventsModel,
                                                                       QObject *parent)
    : QmlProfilerEventRelativesModelProxy(modelManager, eventsModel, parent)
{}

QmlProfilerEventChildrenModelProxy::~QmlProfilerEventChildrenModelProxy()
{}

void QmlProfilerEventChildrenModelProxy::loadData()
{
    clear();
    QmlProfilerDataModel *simpleModel = m_modelManager->qmlModel();
    if (simpleModel->isEmpty())
        return;

    QString rootEventName = tr("<program>");

    // for level computation
    QHash<int, qint64> endtimesPerLevel;
    int level = QmlDebug::Constants::QML_MIN_LEVEL;
    endtimesPerLevel[0] = 0;

    const QSet<QString> eventsInBindingLoop = m_eventsModel->eventsInBindingLoop();

    // compute parent-child relationship and call count
    QHash<int, QString> lastParent;
    const QVector<QmlProfilerDataModel::QmlEventData> eventList = simpleModel->getEvents();
    foreach (const QmlProfilerDataModel::QmlEventData &event, eventList) {
        // whitelist
        if (!m_eventsModel->eventTypeAccepted(event.rangeType))
            continue;

        // level computation
        if (endtimesPerLevel[level] > event.startTime) {
            level++;
        } else {
            while (level > QmlDebug::Constants::QML_MIN_LEVEL && endtimesPerLevel[level-1] <= event.startTime)
                level--;
        }
        endtimesPerLevel[level] = event.startTime + event.duration;

        QString parentHash = rootEventName;
        QString eventHash = QmlProfilerDataModel::getHashString(event);

        if (level > QmlDebug::Constants::QML_MIN_LEVEL && lastParent.contains(level-1))
            parentHash = lastParent[level-1];

        // generate placeholder if needed
        if (!m_data.contains(parentHash))
            m_data.insert(parentHash, QmlEventRelativesMap());

        if (m_data[parentHash].contains(eventHash)) {
            QmlEventRelativesData *child = &(m_data[parentHash][eventHash]);
            child->calls++;
            child->duration += event.duration;
        } else {
            m_data[parentHash].insert(eventHash, QmlEventRelativesData());
            QmlEventRelativesData *child = &(m_data[parentHash][eventHash]);
            child->displayName = event.displayName;
            child->rangeType = event.rangeType;
            child->duration = event.duration;
            child->calls = 1;
            child->details = event.data.join(QLatin1String(""));
            child->isBindingLoop = eventsInBindingLoop.contains(parentHash);
        }

        // now lastparent is a string with the hash
        lastParent[level] = eventHash;
    }
}



}
}
