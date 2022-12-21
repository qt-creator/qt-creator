// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilereventtypes.h"
#include "qmleventlocation.h"
#include "qmlprofiler_global.h"
#include "qmltypedevent.h"

#include <qmldebug/qmldebugclient.h>

#include <QStack>
#include <QStringList>

namespace QmlProfiler {

class QmlProfilerModelManager;
class QmlProfilerTraceClient : public QmlDebug::QmlDebugClient
{
    Q_OBJECT
    Q_PROPERTY(bool recording READ isRecording WRITE setRecording NOTIFY recordingChanged)

public:
    QmlProfilerTraceClient(QmlDebug::QmlDebugConnection *client,
                           QmlProfilerModelManager *modelManager,
                           quint64 features);
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

} // namespace QmlProfiler
