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

void QmlProfilerClientManager::clearBufferedData()
{
    if (m_clientPlugin)
        m_clientPlugin->clearData();
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
                     m_modelManager->traceTime(), &QmlProfilerTraceTime::increaseEndTime);

    QObject::connect(m_profilerState.data(), &QmlProfilerStateManager::requestedFeaturesChanged,
                     m_clientPlugin.data(), &QmlProfilerTraceClient::setRequestedFeatures);
    QObject::connect(m_clientPlugin.data(), &QmlProfilerTraceClient::recordedFeaturesChanged,
                     m_profilerState.data(), &QmlProfilerStateManager::setRecordedFeatures);

    QObject::connect(m_clientPlugin.data(), &QmlProfilerTraceClient::traceStarted,
                     this, [this](qint64 time) {
        m_profilerState->setServerRecording(true);
        m_modelManager->traceTime()->decreaseStartTime(time);
    });

    QObject::connect(m_clientPlugin, &QmlProfilerTraceClient::complete, this, [this](qint64 time) {
        m_modelManager->traceTime()->increaseEndTime(time);
        m_profilerState->setServerRecording(false);
    });

    QObject::connect(m_profilerState.data(), &QmlProfilerStateManager::clientRecordingChanged,
                     m_clientPlugin.data(), &QmlProfilerTraceClient::setRecording);

    QObject::connect(this, &QmlDebug::QmlDebugConnectionManager::connectionOpened,
                     m_clientPlugin.data(), [this]() {
        m_clientPlugin->setRecording(m_profilerState->clientRecording());
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
