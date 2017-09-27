/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <qmldebug/qmldebug_global.h>
#include <qmldebug/qmldebugclient.h>

#include <QPointer>
#include <QTimer>
#include <QUrl>

namespace QmlDebug {

class QMLDEBUG_EXPORT QmlDebugConnectionManager : public QObject
{
    Q_OBJECT
public:
    explicit QmlDebugConnectionManager(QObject *parent = 0);
    ~QmlDebugConnectionManager();

    void connectToServer(const QUrl &server);
    void disconnectFromServer();

    bool isConnected() const;

    void setRetryParams(int interval, int maxAttempts);
    void retryConnect();

signals:
    void connectionOpened();
    void connectionFailed();
    void connectionClosed();

protected:
    virtual void createClients() = 0;
    virtual void destroyClients() = 0;
    virtual void logState(const QString &message);

    QmlDebugConnection *connection() const;

private:
    void connectToTcpServer();
    void startLocalServer();

    QScopedPointer<QmlDebug::QmlDebugConnection> m_connection;
    QTimer m_connectionTimer;
    QUrl m_server;

    int m_retryInterval = 200;
    int m_maximumRetries = 50;
    int m_numRetries = 0;

    void createConnection();
    void destroyConnection();

    void connectConnectionSignals();
    void disconnectConnectionSignals();

    void stopConnectionTimer();

    void qmlDebugConnectionOpened();
    void qmlDebugConnectionClosed();
    void qmlDebugConnectionFailed();
};

} // namespace QmlDebug
