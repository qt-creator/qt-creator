// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    explicit QmlDebugConnectionManager(QObject *parent = nullptr);
    ~QmlDebugConnectionManager() override;

    void connectToServer(const QUrl &server);
    void disconnectFromServer();

    bool isConnecting() const;
    bool isConnected() const;

    int retryInterval() const { return m_retryInterval; }
    void setRetryInterval(int retryInterval) { m_retryInterval = retryInterval; }

    int maximumRetries() const { return m_maximumRetries; }
    void setMaximumRetries(int maximumRetries) { m_maximumRetries = maximumRetries; }

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
    int m_maximumRetries = 10;
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
