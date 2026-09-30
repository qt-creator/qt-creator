// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlrecordingtimeline.h"

#include <qmldebug/qmldebugclient.h>
#include <qmldebug/qmldebugconnectionmanager.h>
#include <qmldebug/qmlprofilertraceclient.h>

#include <QPointer>

#include <functional>
#include <optional>

namespace Profiler::Internal {

class QmlProfilerModelManager;
class QmlProfilerStateManager;

class QmlProfilerClientManager final : public QmlDebug::QmlDebugConnectionManager
{
    Q_OBJECT

public:
    explicit QmlProfilerClientManager(QObject *parent = nullptr);

    void setProfilerStateManager(QmlProfilerStateManager *profilerState);
    void clearEvents();
    void setModelManager(QmlProfilerModelManager *modelManager);
    void setFlushInterval(quint32 flushInterval);
    void clearBufferedData();
    void stopRecording();

    // Whether the server's times are put on the recording's own timeline (see
    // QmlServerTimeRebaser), rather than taken as they come.
    void setRebasingTime(bool rebase) { m_rebasingTime = rebase; }

    // Where connection state messages go. By default they are flashed in the
    // Creator message pane; callers running outside Creator (e.g. the standalone
    // trace viewer, which has no initialized Core) can redirect them, e.g. to
    // qDebug(). The message is already prefixed with "QML Profiler: ".
    void setLogger(const std::function<void(const QString &message)> &logger);

signals:
    // The server began or ended a trace, with the time it gives for it. Ended
    // also for a pause, which the server cannot tell from a stop.
    void traceStartedAt(qint64 time);
    void traceFinishedAt(qint64 time);

private:
    void createClients() final;
    void destroyClients() final;
    void logState(const QString &message) final;

    QPointer<QmlDebug::QmlProfilerTraceClient> m_clientPlugin;
    QPointer<QmlProfilerStateManager> m_profilerState;
    QPointer<QmlProfilerModelManager> m_modelManager;
    std::function<void(const QString &)> m_logger;
    quint32 m_flushInterval = 0;
    bool m_rebasingTime = false;
    QmlServerTimeRebaser m_rebaser;
};

} // namespace Profiler::Internal
