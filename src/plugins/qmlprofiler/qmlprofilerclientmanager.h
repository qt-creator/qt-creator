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
#include <utils/port.h>

#include <QPointer>
#include <QTimer>
#include <QObject>
#include <QVector>

namespace QmlProfiler {
class QmlProfilerModelManager;
class QmlProfilerStateManager;

namespace Internal {

class QmlProfilerClientManager : public QObject
{
    Q_OBJECT
public:
    explicit QmlProfilerClientManager(QObject *parent = 0);
    ~QmlProfilerClientManager();

    void setProfilerStateManager(QmlProfilerStateManager *profilerState);
    void setTcpConnection(QString host, Utils::Port port);
    void setLocalSocket(QString file);
    void clearConnection();

    void clearBufferedData();
    bool isConnected() const;

    void setModelManager(QmlProfilerModelManager *m);
    void setFlushInterval(quint32 flushInterval);

    void setRetryParams(int interval, int maxAttempts);
    void retryConnect();
    void connectToTcpServer();
    void startLocalServer();

    void stopRecording();

signals:
    void connectionOpened();
    void connectionFailed();
    void connectionClosed();

private:
    QPointer<QmlProfilerStateManager> m_profilerState;
    QPointer<QmlProfilerModelManager> m_modelManager;
    QScopedPointer<QmlDebug::QmlDebugConnection> m_connection;
    QScopedPointer<QmlProfilerTraceClient> m_qmlclientplugin;

    QTimer m_connectionTimer;

    QString m_localSocket;
    QString m_tcpHost;
    Utils::Port m_tcpPort;
    quint32 m_flushInterval = 0;

    int m_retryInterval = 200;
    int m_maximumRetries = 50;
    int m_numRetries = 0;

    void disconnectClient();
    void stopConnectionTimer();

    void qmlDebugConnectionOpened();
    void qmlDebugConnectionClosed();
    void qmlDebugConnectionFailed();

    void logState(const QString &);

    void createConnection();
    void connectClientSignals();
    void disconnectClientSignals();
};

} // namespace Internal
} // namespace QmlProfiler
