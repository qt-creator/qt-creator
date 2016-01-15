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
#include "qmlenginecontrolclient.h"
#include "qdebugmessageclient.h"
#include "qpacketprotocol.h"

namespace QmlDebug {

class QmlProfilerTraceClientPrivate {
public:
    QmlProfilerTraceClientPrivate(QmlProfilerTraceClient *_q, QmlDebugConnection *client)
        : q(_q)
        , engineControl(client)
        ,  inProgressRanges(0)
        , maximumTime(0)
        , recording(false)
        , requestedFeatures(0)
        , recordedFeatures(0)
        , flushInterval(0)
    {
        ::memset(rangeCount, 0, MaximumRangeType * sizeof(int));
    }

    void sendRecordingStatus(int engineId);
    bool updateFeatures(QmlDebug::ProfileFeature feature);

    QmlProfilerTraceClient *q;
    QmlEngineControlClient engineControl;
    QScopedPointer<QDebugMessageClient> messageClient;
    qint64 inProgressRanges;
    QStack<qint64> rangeStartTimes[MaximumRangeType];
    QStack<QString> rangeDatas[MaximumRangeType];
    QStack<QmlEventLocation> rangeLocations[MaximumRangeType];
    QStack<BindingType> bindingTypes;
    int rangeCount[MaximumRangeType];
    qint64 maximumTime;
    bool recording;
    quint64 requestedFeatures;
    quint64 recordedFeatures;
    quint32 flushInterval;
};

} // namespace QmlDebug

using namespace QmlDebug;

void QmlProfilerTraceClientPrivate::sendRecordingStatus(int engineId)
{
    QPacket stream(q->connection()->currentDataStreamVersion());
    stream << recording << engineId; // engineId -1 is OK. It means "all of them"
    if (recording)
        stream << requestedFeatures << flushInterval;
    q->sendMessage(stream.data());
}

QmlProfilerTraceClient::QmlProfilerTraceClient(QmlDebugConnection *client, quint64 features)
    : QmlDebugClient(QLatin1String("CanvasFrameRate"), client)
    , d(new QmlProfilerTraceClientPrivate(this, client))
{
    setRequestedFeatures(features);
    connect(&d->engineControl, &QmlEngineControlClient::engineAboutToBeAdded,
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
    ::memset(d->rangeCount, 0, MaximumRangeType * sizeof(int));
    for (int eventType = 0; eventType < MaximumRangeType; eventType++) {
        d->rangeDatas[eventType].clear();
        d->rangeLocations[eventType].clear();
        d->rangeStartTimes[eventType].clear();
    }
    d->bindingTypes.clear();
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
        d->messageClient.reset(new QDebugMessageClient(connection()));
        connect(d->messageClient.data(), &QDebugMessageClient::message, this, [this](QtMsgType type,
                const QString &text, const QmlDebug::QDebugContextInfo &context)
        {
            emit debugMessage(type, context.timestamp, text,
                              QmlDebug::QmlEventLocation(context.file, context.line, 1));
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
    QPacket stream(connection()->currentDataStreamVersion(), data);

    qint64 time;
    int messageType;
    int subtype;

    stream >> time >> messageType;
    if (!stream.atEnd())
        stream >> subtype;
    else
        subtype = -1;

    switch (messageType) {
    case Event: {
        switch (subtype) {
        case StartTrace: {
            if (!d->recording)
                setRecordingFromServer(true);
            QList<int> engineIds;
            while (!stream.atEnd()) {
                int id;
                stream >> id;
                engineIds << id;
            }
            emit traceStarted(time, engineIds);
            d->maximumTime = time;
            break;
        }
        case EndTrace: {
            QList<int> engineIds;
            while (!stream.atEnd()) {
                int id;
                stream >> id;
                engineIds << id;
            }
            emit traceFinished(time, engineIds);
            d->maximumTime = time;
            d->maximumTime = qMax(time, d->maximumTime);
            break;
        }
        case AnimationFrame: {
            if (!d->updateFeatures(ProfileAnimations))
                break;
            int frameRate, animationCount;
            int threadId;
            stream >> frameRate >> animationCount;
            if (!stream.atEnd())
                stream >> threadId;
            else
                threadId = 0;

            emit rangedEvent(Event, MaximumRangeType, AnimationFrame, time, 0, QString(),
                             QmlEventLocation(), frameRate, animationCount, threadId,
                             0, 0);
            d->maximumTime = qMax(time, d->maximumTime);
            break;
        }
        case Key:
        case Mouse:
            if (!d->updateFeatures(ProfileInputEvents))
                break;
            int inputType = (subtype == Key ? InputKeyUnknown : InputMouseUnknown);
            if (!stream.atEnd())
                stream >> inputType;
            int a = -1;
            if (!stream.atEnd())
                stream >> a;
            int b = -1;
            if (!stream.atEnd())
                stream >> b;

            emit rangedEvent(Event, MaximumRangeType, subtype, time, 0, QString(),
                             QmlEventLocation(), inputType, a, b, 0, 0);
            d->maximumTime = qMax(time, d->maximumTime);
            break;
        }

        break;
    }
    case Complete:
        emit complete(d->maximumTime);
        setRecordingFromServer(false);
        break;
    case SceneGraphFrame: {
        if (!d->updateFeatures(ProfileSceneGraph))
            break;

        int count = 0;
        qint64 params[5];

        while (!stream.atEnd()) {
            stream >> params[count++];
        }
        while (count<5)
            params[count++] = 0;
        emit rangedEvent(SceneGraphFrame, MaximumRangeType, subtype,time, 0,
                         QString(), QmlEventLocation(), params[0], params[1],
                         params[2], params[3], params[4]);
        break;
    }
    case PixmapCacheEvent: {
        if (!d->updateFeatures(ProfilePixmapCache))
            break;
        int width = 0, height = 0, refcount = 0;
        QString pixUrl;
        stream >> pixUrl;
        if (subtype == (int)PixmapReferenceCountChanged || subtype == (int)PixmapCacheCountChanged) {
            stream >> refcount;
        } else if (subtype == (int)PixmapSizeKnown) {
            stream >> width >> height;
            refcount = 1;
        }
        emit rangedEvent(PixmapCacheEvent, MaximumRangeType, subtype, time, 0,
                         QString(), QmlEventLocation(pixUrl,0,0), width, height,
                         refcount, 0, 0);
        d->maximumTime = qMax(time, d->maximumTime);
        break;
    }
    case MemoryAllocation: {
        if (!d->updateFeatures(ProfileMemory))
            break;

        qint64 delta;
        stream >> delta;
        emit rangedEvent(MemoryAllocation, MaximumRangeType, subtype, time, 0,
                         QString(), QmlEventLocation(), delta, 0, 0, 0, 0);
        d->maximumTime = qMax(time, d->maximumTime);
        break;
    }
    case RangeStart: {
        if (!d->updateFeatures(featureFromRangeType(static_cast<RangeType>(subtype))))
            break;
        d->rangeStartTimes[subtype].push(time);
        d->inProgressRanges |= (static_cast<qint64>(1) << subtype);
        ++d->rangeCount[subtype];

        // read binding type
        if ((RangeType)subtype == Binding) {
            int bindingType = (int)QmlBinding;
            if (!stream.atEnd())
                stream >> bindingType;
            d->bindingTypes.push((BindingType)bindingType);
        }
        break;
    }
    case RangeData: {
        if (!d->updateFeatures(featureFromRangeType(static_cast<RangeType>(subtype))))
            break;
        QString data;
        stream >> data;

        int count = d->rangeCount[subtype];
        if (count > 0) {
            while (d->rangeDatas[subtype].count() < count)
                d->rangeDatas[subtype].push(QString());
            d->rangeDatas[subtype][count-1] = data;
        }
        break;
    }
    case RangeLocation: {
        if (!d->updateFeatures(featureFromRangeType(static_cast<RangeType>(subtype))))
            break;
        QString fileName;
        int line;
        int column = -1;
        stream >> fileName >> line;

        if (!stream.atEnd())
            stream >> column;

        if (d->rangeCount[subtype] > 0)
            d->rangeLocations[subtype].push(QmlEventLocation(fileName, line, column));
        break;
    }
    case RangeEnd: {
        if (!d->updateFeatures(featureFromRangeType(static_cast<RangeType>(subtype))))
            break;
        if (d->rangeCount[subtype] == 0)
            break;
        --d->rangeCount[subtype];
        if (d->inProgressRanges & (static_cast<qint64>(1) << subtype))
            d->inProgressRanges &= ~(static_cast<qint64>(1) << subtype);

        d->maximumTime = qMax(time, d->maximumTime);
        QString data = d->rangeDatas[subtype].count() ? d->rangeDatas[subtype].pop() : QString();
        QmlEventLocation location = d->rangeLocations[subtype].count() ? d->rangeLocations[subtype].pop() : QmlEventLocation();

        qint64 startTime = d->rangeStartTimes[subtype].pop();
        BindingType bindingType = QmlBinding;
        if ((RangeType)subtype == Binding)
            bindingType = d->bindingTypes.pop();
        if ((RangeType)subtype == Painting)
            bindingType = QPainterEvent;
        emit rangedEvent(MaximumMessage, (RangeType)subtype, bindingType, startTime,
                         time - startTime, data, location, 0, 0, 0, 0, 0);
        if (d->rangeCount[subtype] == 0) {
            int count = d->rangeDatas[subtype].count() +
                        d->rangeStartTimes[subtype].count() +
                        d->rangeLocations[subtype].count();
            if (count != 0)
                qWarning() << "incorrectly nested data";
        }
        break;
    }
    default:
        break;
    }
}
