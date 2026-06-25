// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldebug/qmldebugclient.h>
#include <qmldebug/qmldebugconnectionmanager.h>
#include <qmldebug/qmlprofilertraceclient.h>

#include <QPointer>

#include <functional>

namespace Profiler::Internal {

class QmlProfilerModelManager;
class QmlProfilerStateManager;

class QmlProfilerClientManager final : public QmlDebug::QmlDebugConnectionManager
{
public:
    explicit QmlProfilerClientManager(QObject *parent = nullptr);

    void setProfilerStateManager(QmlProfilerStateManager *profilerState);
    void clearEvents();
    void setModelManager(QmlProfilerModelManager *modelManager);
    void setFlushInterval(quint32 flushInterval);
    void clearBufferedData();
    void stopRecording();

    // Where connection state messages go. By default they are flashed in the
    // Creator message pane; callers running outside Creator (e.g. the standalone
    // trace viewer, which has no initialized Core) can redirect them, e.g. to
    // qDebug(). The message is already prefixed with "QML Profiler: ".
    void setLogger(const std::function<void(const QString &message)> &logger);

private:
    void createClients() final;
    void destroyClients() final;
    void logState(const QString &message) final;

    QPointer<QmlDebug::QmlProfilerTraceClient> m_clientPlugin;
    QPointer<QmlProfilerStateManager> m_profilerState;
    QPointer<QmlProfilerModelManager> m_modelManager;
    std::function<void(const QString &)> m_logger;
    quint32 m_flushInterval = 0;
};

} // namespace Profiler::Internal
