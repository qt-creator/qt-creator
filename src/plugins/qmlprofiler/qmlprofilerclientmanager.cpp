// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerclientmanager.h"
#include "qmlprofilertool.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerstatemanager.h"

#include <utils/qtcassert.h>

namespace QmlProfiler {
namespace Internal {

QmlProfilerClientManager::QmlProfilerClientManager(QObject *parent) :
    QmlDebug::QmlDebugConnectionManager(parent)
{
    setObjectName(QLatin1String("QML Profiler Connections"));
}

void QmlProfilerClientManager::setModelManager(QmlProfilerModelManager *modelManager)
{
    QTC_ASSERT(!connection() && !m_clientPlugin, disconnectFromServer());
    m_modelManager = modelManager;
}

void QmlProfilerClientManager::setProfilerStateManager(QmlProfilerStateManager *profilerState)
{
    // Don't do this while connecting
    QTC_ASSERT(!connection() && !m_clientPlugin, disconnectFromServer());
    m_profilerState = profilerState;
}

void QmlProfilerClientManager::setFlushInterval(quint32 flushInterval)
{
    m_flushInterval = flushInterval;
}

void QmlProfilerClientManager::clearEvents()
{
    if (m_clientPlugin)
        m_clientPlugin->clearEvents();
}

void QmlProfilerClientManager::clearBufferedData()
{
    if (m_clientPlugin)
        m_clientPlugin->clear();
}

void QmlProfilerClientManager::stopRecording()
{
    QTC_ASSERT(m_clientPlugin, return);
    m_clientPlugin->setRecording(false);
}

void QmlProfilerClientManager::createClients()
{
    QTC_ASSERT(m_profilerState, return);
    QTC_ASSERT(m_modelManager, return);
    QTC_ASSERT(!m_clientPlugin, return);

    // false by default (will be set to true when connected)
    m_profilerState->setServerRecording(false);
    m_profilerState->setRecordedFeatures(0);
    m_clientPlugin = new QmlProfilerTraceClient(connection(), m_modelManager,
                                                m_profilerState->requestedFeatures());
    QTC_ASSERT(m_clientPlugin, return);

    m_clientPlugin->setFlushInterval(m_flushInterval);

    QObject::connect(m_clientPlugin.data(), &QmlProfilerTraceClient::traceFinished,
                     m_modelManager, &QmlProfilerModelManager::increaseTraceEnd);

    QObject::connect(m_profilerState.data(), &QmlProfilerStateManager::requestedFeaturesChanged,
                     m_clientPlugin.data(), &QmlProfilerTraceClient::setRequestedFeatures);
    QObject::connect(m_clientPlugin.data(), &QmlProfilerTraceClient::recordedFeaturesChanged,
                     m_profilerState.data(), &QmlProfilerStateManager::setRecordedFeatures);

    QObject::connect(m_clientPlugin.data(), &QmlProfilerTraceClient::traceStarted,
                     this, [this](qint64 time) {
        m_profilerState->setServerRecording(true);
        m_modelManager->decreaseTraceStart(time);
    });

    QObject::connect(m_clientPlugin, &QmlProfilerTraceClient::complete, this, [this](qint64 time) {
        m_modelManager->increaseTraceEnd(time);
        m_profilerState->setServerRecording(false);
    });

    QObject::connect(m_profilerState.data(), &QmlProfilerStateManager::clientRecordingChanged,
                     m_clientPlugin.data(), &QmlProfilerTraceClient::setRecording);

    QObject::connect(this, &QmlDebug::QmlDebugConnectionManager::connectionOpened,
                     m_clientPlugin.data(), [this]() {
        m_clientPlugin->setRecording(m_profilerState->clientRecording());
    });
    QObject::connect(this, &QmlDebug::QmlDebugConnectionManager::connectionClosed,
                     m_clientPlugin.data(), [this]() {
        m_profilerState->setServerRecording(false);
    });
}

void QmlProfilerClientManager::destroyClients()
{
    QTC_ASSERT(m_clientPlugin, return);
    m_clientPlugin->disconnect();
    m_clientPlugin->deleteLater();

    QTC_ASSERT(m_profilerState, return);
    QObject::disconnect(m_profilerState.data(), &QmlProfilerStateManager::requestedFeaturesChanged,
                        m_clientPlugin.data(), &QmlProfilerTraceClient::setRequestedFeatures);
    QObject::disconnect(m_profilerState.data(), &QmlProfilerStateManager::clientRecordingChanged,
                        m_clientPlugin.data(), &QmlProfilerTraceClient::setRecording);
    m_clientPlugin.clear();
}

void QmlProfilerClientManager::logState(const QString &message)
{
    QmlProfilerTool::logState(QLatin1String("QML Profiler: ") + message);
}

} // namespace Internal
} // namespace QmlProfiler
