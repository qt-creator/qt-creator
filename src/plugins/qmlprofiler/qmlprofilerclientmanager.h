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
