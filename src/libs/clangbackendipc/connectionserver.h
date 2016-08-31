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

#include "clangbackendipc_global.h"

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

struct CMBIPC_EXPORT ConnectionName {
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

        aliveTimer.start(5000);

        localServer.setMaxPendingConnections(1);

        QObject::connect(&localServer,
                         &QLocalServer::newConnection,
                         [&] { handleNewConnection(); });
        QObject::connect(&aliveTimer,
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
        localServer.listen(ConnectionName::connectionName);
    }

    void setServer(ServerInterface *ipcServer)
    {
        this->ipcServer = ipcServer;

    }

    static void removeServer()
    {
        QLocalServer::removeServer(ConnectionName::connectionName);
    }

private:
    void handleNewConnection()
    {
        localSocket = nextPendingConnection();

        ipcClientProxy.reset(new ClientProxy(ipcServer, localSocket));

        ipcServer->setClient(ipcClientProxy.get());
    }

    void sendAliveMessage()
    {
        if (ipcClientProxy)
            ipcClientProxy->alive();
    }

    void handleSocketDisconnect()
    {
        ipcClientProxy.reset();

        localSocket = nullptr;

        delayedExitApplicationIfNoSockedIsConnected();
    }

    QLocalSocket *nextPendingConnection()
    {
        QLocalSocket *localSocket = localServer.nextPendingConnection();

        QObject::connect(localSocket,
                         &QLocalSocket::disconnected,
                         [&] { handleSocketDisconnect(); });

        return localSocket;
    }

    void delayedExitApplicationIfNoSockedIsConnected()
    {
        if (localSocket == nullptr)
            QTimer::singleShot(60000, [&] { exitApplicationIfNoSockedIsConnected(); });
    }

    void exitApplicationIfNoSockedIsConnected()
    {
        if (localSocket == nullptr)
            QCoreApplication::exit();
    }

private:
    std::unique_ptr<ClientProxy> ipcClientProxy;
    QLocalSocket* localSocket;
    ServerInterface *ipcServer;
    QLocalServer localServer;
    QTimer aliveTimer;
};

} // namespace ClangBackEnd
