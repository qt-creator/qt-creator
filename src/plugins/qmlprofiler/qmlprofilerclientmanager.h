// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldebug/qmldebugclient.h>
#include <qmldebug/qmldebugconnectionmanager.h>
#include <qmldebug/qmlprofilertraceclient.h>

#include <QPointer>

namespace QmlProfiler::Internal {

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

private:
    void createClients() final;
    void destroyClients() final;
    void logState(const QString &message) final;

    QPointer<QmlDebug::QmlProfilerTraceClient> m_clientPlugin;
    QPointer<QmlProfilerStateManager> m_profilerState;
    QPointer<QmlProfilerModelManager> m_modelManager;
    quint32 m_flushInterval = 0;
};

} // namespace QmlProfiler::Internal
