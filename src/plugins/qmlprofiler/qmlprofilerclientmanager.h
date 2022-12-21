// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilertraceclient.h"

#include <qmldebug/qmldebugclient.h>
#include <qmldebug/qmldebugconnectionmanager.h>

#include <QPointer>
#include <QTimer>
#include <QUrl>

namespace QmlProfiler {
class QmlProfilerModelManager;
class QmlProfilerStateManager;

namespace Internal {

class QmlProfilerClientManager : public QmlDebug::QmlDebugConnectionManager
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

protected:
    void createClients() override;
    void destroyClients() override;
    void logState(const QString &message) override;

private:
    QPointer<QmlProfilerTraceClient> m_clientPlugin;
    QPointer<QmlProfilerStateManager> m_profilerState;
    QPointer<QmlProfilerModelManager> m_modelManager;
    quint32 m_flushInterval = 0;
};

} // namespace Internal
} // namespace QmlProfiler
