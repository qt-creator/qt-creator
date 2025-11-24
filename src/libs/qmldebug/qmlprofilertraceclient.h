// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldebug_global.h"

#include "qmldebugclient.h"
#include "qmleventlocation.h"
#include "qmlprofilereventtypes.h"
#include "qmltypedevent.h"

#include <QStack>
#include <QStringList>

namespace QmlDebug {

class QMLDEBUG_EXPORT QmlProfilerTraceClient : public QmlDebugClient
{
    Q_OBJECT
    Q_PROPERTY(bool recording READ isRecording WRITE setRecording NOTIFY recordingChanged)

public:
    using EventTypeLoader = std::function<int(QmlEventType &&)>;
    using EventLoader = std::function<void(QmlEvent &&)>;

    QmlProfilerTraceClient(
            QmlDebugConnection *client, const EventTypeLoader &eventTypeLoader,
            const EventLoader &eventLoader, quint64 features);
    ~QmlProfilerTraceClient() override;

    bool isRecording() const;
    void setRecording(bool);
    quint64 recordedFeatures() const;
    void messageReceived(const QByteArray &) override;
    void stateChanged(State status) override;

    void clearEvents();
    void clear();
    void sendRecordingStatus(int engineId = -1);
    void setRequestedFeatures(quint64 features);
    void setFlushInterval(quint32 flushInterval);

signals:
    void complete(qint64 maximumTime);
    void traceFinished(qint64 timestamp, const QList<int> &engineIds);
    void traceStarted(qint64 timestamp, const QList<int> &engineIds);

    void recordingChanged(bool arg);
    void recordedFeaturesChanged(quint64 features);

    void cleared();

private:
    class QmlProfilerTraceClientPrivate *d;
};

} // namespace QmlDebug
