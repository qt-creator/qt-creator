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

#include "clangsupport_global.h"

#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <QDebug>

#include <cstdlib>
#include <memory>
#include <vector>

namespace ClangBackEnd {

class ClangCodeModelServerInterface;
class ClangCodeModelClientProxy;

template <typename ServerInterface,
          typename ClientProxy>
class ConnectionServer
{
public:
    ConnectionServer()
    {
        m_aliveTimer.start(5000);

        connectAliveTimer();
        connectLocalSocketDisconnet();
    }

    ~ConnectionServer()
    {
        if (m_localSocket.state() != QLocalSocket::UnconnectedState)
            m_localSocket.disconnectFromServer();
    }

    void start(const QString &connectionName)
    {
        connectToLocalServer(connectionName);
    }

    void setServer(ServerInterface *ipcServer)
    {
        this->m_ipcServer = ipcServer;

    }

    void ensureAliveMessageIsSent()
    {
        if (m_aliveTimer.remainingTime() == 0)
            sendAliveMessage();
    }

private:
    void connectToLocalServer(const QString &connectionName)
    {
        constexpr void (QLocalSocket::*LocalSocketErrorFunction)(QLocalSocket::LocalSocketError)
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                = &QLocalSocket::error;
#else
                = &QLocalSocket::errorOccurred;
#endif

        QObject::connect(&m_localSocket,
                         LocalSocketErrorFunction,
                         [&] (QLocalSocket::LocalSocketError) {
            qWarning() << "ConnectionServer error:" << m_localSocket.errorString() << connectionName;
        });

        m_localSocket.connectToServer(connectionName);
        m_ipcClientProxy = std::make_unique<ClientProxy>(m_ipcServer, &m_localSocket);

        m_ipcServer->setClient(m_ipcClientProxy.get());
    }

    void sendAliveMessage()
    {
        if (m_ipcClientProxy)
            m_ipcClientProxy->alive();
    }

    void handleSocketDisconnect()
    {
        m_ipcClientProxy.reset();

        delayedExitApplicationIfNoSockedIsConnected();
    }

    void delayedExitApplicationIfNoSockedIsConnected()
    {
        QTimer::singleShot(60000, [&] { exitApplicationIfNoSockedIsConnected(); });
    }

    void exitApplicationIfNoSockedIsConnected()
    {
        if (m_localSocket.state() != QLocalSocket::UnconnectedState)
            m_localSocket.disconnectFromServer();
        QCoreApplication::exit();
    }

    void connectAliveTimer()
    {
        QObject::connect(&m_aliveTimer,
                         &QTimer::timeout,
                         [&] { sendAliveMessage(); });
    }

    void connectLocalSocketDisconnet()
    {
        QObject::connect(&m_localSocket,
                         &QLocalSocket::disconnected,
                         [&] { handleSocketDisconnect(); });
    }

private:
    QLocalSocket m_localSocket;
    QTimer m_aliveTimer;
    std::unique_ptr<ClientProxy> m_ipcClientProxy;
    ServerInterface *m_ipcServer = nullptr;
};

} // namespace ClangBackEnd
