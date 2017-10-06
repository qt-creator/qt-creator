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

#include <cstdlib>
#include <memory>
#include <vector>

namespace ClangBackEnd {

class ClangCodeModelServerInterface;
class ClangCodeModelClientProxy;

struct CLANGSUPPORT_EXPORT ConnectionName {
    static QString connectionName;
};

template <typename ServerInterface,
          typename ClientProxy>
class ConnectionServer
{
public:
    ConnectionServer(const QString &connectionName)
    {
        ConnectionName::connectionName = connectionName;

        m_aliveTimer.start(5000);

        m_localServer.setMaxPendingConnections(1);

        QObject::connect(&m_localServer,
                         &QLocalServer::newConnection,
                         [&] { handleNewConnection(); });
        QObject::connect(&m_aliveTimer,
                         &QTimer::timeout,
                         [&] { sendAliveMessage(); });

        std::atexit(&ConnectionServer::removeServer);
    #if defined(_GLIBCXX_HAVE_AT_QUICK_EXIT)
        std::at_quick_exit(&ConnectionServer::removeServer);
    #endif
        std::set_terminate(&ConnectionServer::removeServer);
    }

    ~ConnectionServer()
    {
        removeServer();
    }

    void start()
    {
        QLocalServer::removeServer(ConnectionName::connectionName);
        m_localServer.listen(ConnectionName::connectionName);
    }

    void setServer(ServerInterface *ipcServer)
    {
        this->m_ipcServer = ipcServer;

    }

    static void removeServer()
    {
        QLocalServer::removeServer(ConnectionName::connectionName);
    }

private:
    void handleNewConnection()
    {
        m_localSocket = nextPendingConnection();

        m_ipcClientProxy.reset(new ClientProxy(m_ipcServer, m_localSocket));

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

        m_localSocket = nullptr;

        delayedExitApplicationIfNoSockedIsConnected();
    }

    QLocalSocket *nextPendingConnection()
    {
        QLocalSocket *localSocket = m_localServer.nextPendingConnection();

        QObject::connect(localSocket,
                         &QLocalSocket::disconnected,
                         [&] { handleSocketDisconnect(); });

        return localSocket;
    }

    void delayedExitApplicationIfNoSockedIsConnected()
    {
        if (m_localSocket == nullptr)
            QTimer::singleShot(60000, [&] { exitApplicationIfNoSockedIsConnected(); });
    }

    void exitApplicationIfNoSockedIsConnected()
    {
        if (m_localSocket == nullptr)
            QCoreApplication::exit();
    }

private:
    std::unique_ptr<ClientProxy> m_ipcClientProxy;
    QLocalSocket* m_localSocket;
    ServerInterface *m_ipcServer;
    QLocalServer m_localServer;
    QTimer m_aliveTimer;
};

} // namespace ClangBackEnd
