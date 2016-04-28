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

#include <qmldebug/qmlenginecontrolclient.h>
#include <qmldebug/qdebugmessageclient.h>
#include <qmldebug/qpacketprotocol.h>

namespace QmlProfiler {

class QmlProfilerTraceClientPrivate {
public:
    QmlProfilerTraceClientPrivate(QmlProfilerTraceClient *_q, QmlDebug::QmlDebugConnection *client)
        : q(_q)
        , engineControl(client)
        , maximumTime(0)
        , recording(false)
        , requestedFeatures(0)
        , recordedFeatures(0)
        , flushInterval(0)
    {
    }

    void sendRecordingStatus(int engineId);
    bool updateFeatures(ProfileFeature feature);

    QmlProfilerTraceClient *q;
    QmlDebug::QmlEngineControlClient engineControl;
    QScopedPointer<QmlDebug::QDebugMessageClient> messageClient;
    qint64 maximumTime;
    bool recording;
    quint64 requestedFeatures;
    quint64 recordedFeatures;
    quint32 flushInterval;

    // Reuse the same event, so that we don't have to constantly reallocate all the data.
    QmlTypedEvent currentEvent;
};

void QmlProfilerTraceClientPrivate::sendRecordingStatus(int engineId)
{
    QmlDebug::QPacket stream(q->connection()->currentDataStreamVersion());
    stream << recording << engineId; // engineId -1 is OK. It means "all of them"
    if (recording)
        stream << requestedFeatures << flushInterval;
    q->sendMessage(stream.data());
}

QmlProfilerTraceClient::QmlProfilerTraceClient(QmlDebug::QmlDebugConnection *client,
                                               quint64 features)
    : QmlDebugClient(QLatin1String("CanvasFrameRate"), client)
    , d(new QmlProfilerTraceClientPrivate(this, client))
{
    setRequestedFeatures(features);
    connect(&d->engineControl, &QmlDebug::QmlEngineControlClient::engineAboutToBeAdded,
            this, &QmlProfilerTraceClient::newEngine);
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
            d->currentEvent.type.location.filename = context.file;
            d->currentEvent.type.location.line = context.line;
            d->currentEvent.type.location.column = 1;
            d->currentEvent.type.displayName.clear();
            d->currentEvent.type.data.clear();
            d->currentEvent.type.message = DebugMessage;
            d->currentEvent.type.rangeType = MaximumRangeType;
            d->currentEvent.type.detailType = type;
            emit qmlEvent(d->currentEvent.event, d->currentEvent.type);
        });
    } else {
        d->messageClient.reset();
    }
}

void QmlProfilerTraceClient::setFlushInterval(quint32 flushInterval)
{
    d->flushInterval = flushInterval;
}

void QmlProfilerTraceClient::setRecordingFromServer(bool v)
{
    if (v == d->recording)
        return;
    d->recording = v;
    emit recordingChanged(v);
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
    if (d->currentEvent.type.message == Complete) {
        emit complete(d->maximumTime);
        setRecordingFromServer(false);
    } else if (d->currentEvent.type.message == Event
               && d->currentEvent.type.detailType == StartTrace) {
        if (!d->recording)
            setRecordingFromServer(true);
        emit traceStarted(d->currentEvent.event.timestamp(),
                          d->currentEvent.event.numbers<QList<int>, qint32>());
    } else if (d->currentEvent.type.message == Event
               && d->currentEvent.type.detailType == EndTrace) {
        emit traceFinished(d->currentEvent.event.timestamp(),
                           d->currentEvent.event.numbers<QList<int>, qint32>());
    } else if (d->updateFeatures(d->currentEvent.type.feature())) {
        emit qmlEvent(d->currentEvent.event, d->currentEvent.type);
    }
}

} // namespace QmlProfiler
