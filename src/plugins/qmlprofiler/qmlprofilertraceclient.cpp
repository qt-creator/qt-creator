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

#include "qmlprofilertraceclient.h"
#include "qmltypedevent.h"
#include "qmlprofilermodelmanager.h"

#include <qmldebug/qmlenginecontrolclient.h>
#include <qmldebug/qdebugmessageclient.h>
#include <qmldebug/qpacketprotocol.h>
#include <utils/qtcassert.h>

#include <QQueue>

namespace QmlProfiler {

inline uint qHash(const QmlEventType &type)
{
    return qHash(type.location())
            ^ (((type.message() << 12) & 0xf000)             // 4 bits of message
               | ((type.rangeType() << 24) & 0xf000000)      // 4 bits of rangeType
               | ((type.detailType() << 28) & 0xf0000000));  // 4 bits of detailType
}

inline bool operator==(const QmlEventType &type1, const QmlEventType &type2)
{
    return type1.message() == type2.message() && type1.rangeType() == type2.rangeType()
            && type1.detailType() == type2.detailType() && type1.location() == type2.location();
}

inline bool operator!=(const QmlEventType &type1, const QmlEventType &type2)
{
    return !(type1 == type2);
}

struct ObjectDeleteLater { void operator()(QObject *o) { o->deleteLater(); } };

class QmlProfilerTraceClientPrivate {
public:
    QmlProfilerTraceClientPrivate(QmlProfilerTraceClient *q,
                                  QmlDebug::QmlDebugConnection *connection,
                                  QmlProfilerModelManager *modelManager)
        : q(q)
        , modelManager(modelManager)
        , engineControl(new QmlDebug::QmlEngineControlClient(connection))
        , maximumTime(0)
        , recording(false)
        , requestedFeatures(0)
        , recordedFeatures(0)
        , flushInterval(0)
    {
    }

    void sendRecordingStatus(int engineId);
    bool updateFeatures(quint8 feature);
    int resolveType(const QmlTypedEvent &type);
    int resolveStackTop();
    void forwardEvents(QmlEvent &&last);
    void processCurrentEvent();
    void finalize();

    QmlProfilerTraceClient *q;
    QmlProfilerModelManager *modelManager;

    // Use deleteLater here. The connection will call stateChanged() on all clients that are
    // alive when it gets disconnected. One way to notice a disconnection is failing to send the
    // plugin advertisement when a client unregisters. If one of the other clients is
    // half-destructed at that point, we get invalid memory accesses. Therefore, we cannot nest the
    // dtor calls.
    std::unique_ptr<QmlDebug::QmlEngineControlClient, ObjectDeleteLater> engineControl;
    std::unique_ptr<QmlDebug::QDebugMessageClient, ObjectDeleteLater> messageClient;
    qint64 maximumTime;
    bool recording;
    quint64 requestedFeatures;
    quint64 recordedFeatures;
    quint32 flushInterval;

    // Reuse the same event, so that we don't have to constantly reallocate all the data.
    QmlTypedEvent currentEvent;
    QHash<QmlEventType, int> eventTypeIds;
    QHash<qint64, int> serverTypeIds;
    QStack<QmlTypedEvent> rangesInProgress;
    QQueue<QmlEvent> pendingMessages;
    QQueue<QmlEvent> pendingDebugMessages;

    QList<int> trackedEngines;
};

int QmlProfilerTraceClientPrivate::resolveType(const QmlTypedEvent &event)
{
    int typeIndex = -1;
    if (event.serverTypeId != 0) {
        QHash<qint64, int>::ConstIterator it = serverTypeIds.constFind(event.serverTypeId);

        if (it != serverTypeIds.constEnd()) {
            typeIndex = it.value();
        } else {
            // We can potentially move the type away here, as we don't need to access it anymore,
            // but that requires some more refactoring.
            typeIndex = modelManager->appendEventType(QmlEventType(event.type));
            serverTypeIds[event.serverTypeId] = typeIndex;
        }
    } else {
        QHash<QmlEventType, int>::ConstIterator it = eventTypeIds.constFind(event.type);

        if (it != eventTypeIds.constEnd()) {
            typeIndex = it.value();
        } else {
            typeIndex = modelManager->appendEventType(QmlEventType(event.type));
            eventTypeIds[event.type] = typeIndex;
        }
    }
    return typeIndex;
}

int QmlProfilerTraceClientPrivate::resolveStackTop()
{
    if (rangesInProgress.isEmpty())
        return -1;

    QmlTypedEvent &typedEvent = rangesInProgress.top();
    int typeIndex = typedEvent.event.typeIndex();
    if (typeIndex >= 0)
        return typeIndex;

    typeIndex = resolveType(typedEvent);
    typedEvent.event.setTypeIndex(typeIndex);
    while (!pendingMessages.isEmpty()
           && pendingMessages.head().timestamp() < typedEvent.event.timestamp()) {
        forwardEvents(pendingMessages.dequeue());
    }
    forwardEvents(QmlEvent(typedEvent.event));
    return typeIndex;
}

void QmlProfilerTraceClientPrivate::forwardEvents(QmlEvent &&last)
{
    while (!pendingDebugMessages.isEmpty()
           && pendingDebugMessages.front().timestamp() <= last.timestamp()) {
         modelManager->appendEvent(pendingDebugMessages.dequeue());
    }
    modelManager->appendEvent(std::move(last));
}

void QmlProfilerTraceClientPrivate::processCurrentEvent()
{
    // RangeData and RangeLocation always apply to the range on the top of the stack. Furthermore,
    // all ranges are perfectly nested. This is why we can defer the type resolution until either
    // the range ends or a child range starts. With only the information in RangeStart we wouldn't
    // be able to uniquely identify the event type.
    Message rangeStage = currentEvent.type.rangeType() == MaximumRangeType ?
                currentEvent.type.message() : currentEvent.event.rangeStage();
    switch (rangeStage) {
    case RangeStart:
        resolveStackTop();
        rangesInProgress.push(currentEvent);
        break;
    case RangeEnd: {
        int typeIndex = resolveStackTop();
        if (typeIndex == -1)
            break;
        currentEvent.event.setTypeIndex(typeIndex);
        while (!pendingMessages.isEmpty())
            forwardEvents(pendingMessages.dequeue());
        forwardEvents(QmlEvent(currentEvent.event));
        rangesInProgress.pop();
        break;
    }
    case RangeData:
        if (!rangesInProgress.isEmpty())
            rangesInProgress.top().type.setData(currentEvent.type.data());
        break;
    case RangeLocation:
        if (!rangesInProgress.isEmpty())
            rangesInProgress.top().type.setLocation(currentEvent.type.location());
        break;
    case DebugMessage:
        currentEvent.event.setTypeIndex(resolveType(currentEvent));
        pendingDebugMessages.enqueue(currentEvent.event);
        break;
    default: {
        int typeIndex = resolveType(currentEvent);
        currentEvent.event.setTypeIndex(typeIndex);
        if (rangesInProgress.isEmpty())
            forwardEvents(QmlEvent(currentEvent.event));
        else
            pendingMessages.enqueue(currentEvent.event);
        break;
    }
    }
}

void QmlProfilerTraceClientPrivate::finalize()
{
    while (!rangesInProgress.isEmpty()) {
        currentEvent = rangesInProgress.top();
        currentEvent.event.setRangeStage(RangeEnd);
        currentEvent.event.setTimestamp(maximumTime);
        processCurrentEvent();
    }
    QTC_CHECK(pendingMessages.isEmpty());
    while (!pendingDebugMessages.isEmpty())
        modelManager->appendEvent(pendingDebugMessages.dequeue());
}

void QmlProfilerTraceClientPrivate::sendRecordingStatus(int engineId)
{
    QmlDebug::QPacket stream(q->dataStreamVersion());
    stream << recording << engineId; // engineId -1 is OK. It means "all of them"
    if (recording) {
        stream << requestedFeatures << flushInterval;
        stream << true; // yes, we support type IDs
    }
    q->sendMessage(stream.data());
}

QmlProfilerTraceClient::QmlProfilerTraceClient(QmlDebug::QmlDebugConnection *client,
                                               QmlProfilerModelManager *modelManager,
                                               quint64 features)
    : QmlDebugClient(QLatin1String("CanvasFrameRate"), client)
    , d(new QmlProfilerTraceClientPrivate(this, client, modelManager))
{
    setRequestedFeatures(features);
    connect(d->engineControl.get(), &QmlDebug::QmlEngineControlClient::engineAboutToBeAdded,
            this, &QmlProfilerTraceClient::sendRecordingStatus);
    connect(d->engineControl.get(), &QmlDebug::QmlEngineControlClient::engineAboutToBeRemoved,
            this, [this](int engineId) {
        // We may already be done with that engine. Then we don't need to block it.
        if (d->trackedEngines.contains(engineId))
            d->engineControl->blockEngine(engineId);
    });
    connect(this, &QmlProfilerTraceClient::traceFinished,
            d->engineControl.get(), [this](qint64 timestamp, const QList<int> &engineIds) {
        Q_UNUSED(timestamp)
        // The engines might not be blocked because the trace can get finished before engine control
        // sees them.
        for (int blocked : d->engineControl->blockedEngines()) {
            if (engineIds.contains(blocked))
                d->engineControl->releaseEngine(blocked);
        }
    });
}

QmlProfilerTraceClient::~QmlProfilerTraceClient()
{
    //Disable profiling if started by client
    //Profiling data will be lost!!
    if (isRecording())
        setRecording(false);
    delete d;
}

void QmlProfilerTraceClient::clearEvents()
{
    d->rangesInProgress.clear();
    d->pendingMessages.clear();
    d->pendingDebugMessages.clear();
    if (d->recordedFeatures != 0) {
        d->recordedFeatures = 0;
        emit recordedFeaturesChanged(0);
    }
    emit cleared();
}

void QmlProfilerTraceClient::clear()
{
    d->eventTypeIds.clear();
    d->serverTypeIds.clear();
    d->trackedEngines.clear();
    clearEvents();
}

void QmlProfilerTraceClient::sendRecordingStatus(int engineId)
{
    d->sendRecordingStatus(engineId);
}

bool QmlProfilerTraceClient::isRecording() const
{
    return d->recording;
}

void QmlProfilerTraceClient::setRecording(bool v)
{
    if (v == d->recording)
        return;

    d->recording = v;

    if (state() == Enabled)
        sendRecordingStatus();

    emit recordingChanged(v);
}

quint64 QmlProfilerTraceClient::recordedFeatures() const
{
    return d->recordedFeatures;
}

void QmlProfilerTraceClient::setRequestedFeatures(quint64 features)
{
    d->requestedFeatures = features;
    if (features & static_cast<quint64>(1) << ProfileDebugMessages) {
        d->messageClient.reset(new QmlDebug::QDebugMessageClient(connection()));
        connect(d->messageClient.get(), &QmlDebug::QDebugMessageClient::message, this,
                [this](QtMsgType type, const QString &text,
                       const QmlDebug::QDebugContextInfo &context)
        {
            QTC_ASSERT(d->updateFeatures(ProfileDebugMessages), return);
            d->currentEvent.event.setTimestamp(context.timestamp > 0 ? context.timestamp : 0);
            d->currentEvent.event.setTypeIndex(-1);
            d->currentEvent.event.setString(text);
            d->currentEvent.type = QmlEventType(DebugMessage, MaximumRangeType, type,
                                                QmlEventLocation(context.file, context.line, 1));
            d->currentEvent.serverTypeId = 0;
            d->processCurrentEvent();
        });
    } else {
        d->messageClient.reset();
    }
}

void QmlProfilerTraceClient::setFlushInterval(quint32 flushInterval)
{
    d->flushInterval = flushInterval;
}

bool QmlProfilerTraceClientPrivate::updateFeatures(quint8 feature)
{
    quint64 flag = 1ULL << feature;
    if (!(requestedFeatures & flag))
        return false;
    if (!(recordedFeatures & flag)) {
        recordedFeatures |= flag;
        emit q->recordedFeaturesChanged(recordedFeatures);
    }
    return true;
}

void QmlProfilerTraceClient::stateChanged(State status)
{
    if (status == Enabled)
        sendRecordingStatus(-1);
    else
        d->finalize();
}

void QmlProfilerTraceClient::messageReceived(const QByteArray &data)
{
    QmlDebug::QPacket stream(dataStreamVersion(), data);

    stream >> d->currentEvent;

    d->maximumTime = qMax(d->currentEvent.event.timestamp(), d->maximumTime);
    if (d->currentEvent.type.message() == Complete) {
        d->finalize();
        emit complete(d->maximumTime);
    } else if (d->currentEvent.type.message() == Event
               && d->currentEvent.type.detailType() == StartTrace) {
        const QList<int> engineIds = d->currentEvent.event.numbers<QList<int>, qint32>();
        d->trackedEngines.append(engineIds);
        emit traceStarted(d->currentEvent.event.timestamp(), engineIds);
    } else if (d->currentEvent.type.message() == Event
               && d->currentEvent.type.detailType() == EndTrace) {
        const QList<int> engineIds = d->currentEvent.event.numbers<QList<int>, qint32>();
        for (int engineId : engineIds)
            d->trackedEngines.removeAll(engineId);
        emit traceFinished(d->currentEvent.event.timestamp(), engineIds);
    } else if (d->updateFeatures(d->currentEvent.type.feature())) {
        d->processCurrentEvent();
    }
}

} // namespace QmlProfiler
