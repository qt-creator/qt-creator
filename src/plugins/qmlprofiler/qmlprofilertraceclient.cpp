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

class QmlProfilerTraceClientPrivate {
public:
    QmlProfilerTraceClientPrivate(QmlProfilerTraceClient *q,
                                  QmlDebug::QmlDebugConnection *connection,
                                  QmlProfilerModelManager *modelManager)
        : q(q)
        , modelManager(modelManager)
        , engineControl(connection)
        , maximumTime(0)
        , recording(false)
        , requestedFeatures(0)
        , recordedFeatures(0)
        , flushInterval(0)
    {
    }

    void sendRecordingStatus(int engineId);
    bool updateFeatures(ProfileFeature feature);
    int resolveType(const QmlTypedEvent &type);
    int resolveStackTop();
    void processCurrentEvent();

    QmlProfilerTraceClient *q;
    QmlProfilerModelManager *modelManager;
    QmlDebug::QmlEngineControlClient engineControl;
    QScopedPointer<QmlDebug::QDebugMessageClient> messageClient;
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
};

int QmlProfilerTraceClientPrivate::resolveType(const QmlTypedEvent &event)
{
    int typeIndex = -1;
    if (event.serverTypeId != 0) {
        QHash<qint64, int>::ConstIterator it = serverTypeIds.constFind(event.serverTypeId);

        if (it != serverTypeIds.constEnd()) {
            typeIndex = it.value();
        } else {
            typeIndex = modelManager->numLoadedEventTypes();
            modelManager->addEventType(event.type);
            serverTypeIds[event.serverTypeId] = typeIndex;
        }
    } else {
        QHash<QmlEventType, int>::ConstIterator it = eventTypeIds.constFind(event.type);

        if (it != eventTypeIds.constEnd()) {
            typeIndex = it.value();
        } else {
            typeIndex = modelManager->numLoadedEventTypes();
            modelManager->addEventType(event.type);
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
        modelManager->addEvent(pendingMessages.dequeue());
    }
    modelManager->addEvent(typedEvent.event);
    return typeIndex;
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
        QTC_ASSERT(typeIndex != -1, break);
        currentEvent.event.setTypeIndex(typeIndex);
        while (!pendingMessages.isEmpty())
            modelManager->addEvent(pendingMessages.dequeue());
        modelManager->addEvent(currentEvent.event);
        rangesInProgress.pop();
        break;
    }
    case RangeData:
        rangesInProgress.top().type.setData(currentEvent.type.data());
        break;
    case RangeLocation:
        rangesInProgress.top().type.setLocation(currentEvent.type.location());
        break;
    default: {
        int typeIndex = resolveType(currentEvent);
        currentEvent.event.setTypeIndex(typeIndex);
        if (rangesInProgress.isEmpty())
            modelManager->addEvent(currentEvent.event);
        else
            pendingMessages.enqueue(currentEvent.event);
        break;
    }
    }
}

void QmlProfilerTraceClientPrivate::sendRecordingStatus(int engineId)
{
    QmlDebug::QPacket stream(q->connection()->currentDataStreamVersion());
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
    connect(&d->engineControl, &QmlDebug::QmlEngineControlClient::engineAboutToBeAdded,
            this, &QmlProfilerTraceClient::sendRecordingStatus);
}

QmlProfilerTraceClient::~QmlProfilerTraceClient()
{
    //Disable profiling if started by client
    //Profiling data will be lost!!
    if (isRecording())
        setRecording(false);
    delete d;
}

void QmlProfilerTraceClient::clearData()
{
    d->eventTypeIds.clear();
    d->rangesInProgress.clear();
    if (d->recordedFeatures != 0) {
        d->recordedFeatures = 0;
        emit recordedFeaturesChanged(0);
    }
    emit cleared();
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
        connect(d->messageClient.data(), &QmlDebug::QDebugMessageClient::message, this,
                [this](QtMsgType type, const QString &text,
                       const QmlDebug::QDebugContextInfo &context)
        {
            d->updateFeatures(ProfileDebugMessages);
            d->currentEvent.event.setTimestamp(context.timestamp);
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

bool QmlProfilerTraceClientPrivate::updateFeatures(ProfileFeature feature)
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
}

void QmlProfilerTraceClient::messageReceived(const QByteArray &data)
{
    QmlDebug::QPacket stream(connection()->currentDataStreamVersion(), data);

    stream >> d->currentEvent;

    d->maximumTime = qMax(d->currentEvent.event.timestamp(), d->maximumTime);
    if (d->currentEvent.type.message() == Complete) {
        while (!d->rangesInProgress.isEmpty()) {
            d->currentEvent = d->rangesInProgress.top();
            d->currentEvent.event.setRangeStage(RangeEnd);
            d->currentEvent.event.setTimestamp(d->maximumTime);
            d->processCurrentEvent();
        }
        emit complete(d->maximumTime);
    } else if (d->currentEvent.type.message() == Event
               && d->currentEvent.type.detailType() == StartTrace) {
        emit traceStarted(d->currentEvent.event.timestamp(),
                          d->currentEvent.event.numbers<QList<int>, qint32>());
    } else if (d->currentEvent.type.message() == Event
               && d->currentEvent.type.detailType() == EndTrace) {
        emit traceFinished(d->currentEvent.event.timestamp(),
                           d->currentEvent.event.numbers<QList<int>, qint32>());
    } else if (d->updateFeatures(d->currentEvent.type.feature())) {
        d->processCurrentEvent();
    }
}

} // namespace QmlProfiler
