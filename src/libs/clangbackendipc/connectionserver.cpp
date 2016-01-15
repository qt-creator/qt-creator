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

#include "connectionserver.h"

#include <ipcserverinterface.h>

#include <QCoreApplication>
#include <QLocalSocket>
#include <QTimer>

#include <cstdlib>

namespace ClangBackEnd {

QString ConnectionServer::connectionName;

ConnectionServer::ConnectionServer(const QString &connectionName)
    : aliveTimerId(startTimer(5000))
{
    this->connectionName = connectionName;

    connect(&localServer, &QLocalServer::newConnection, this, &ConnectionServer::handleNewConnection);
    std::atexit(&ConnectionServer::removeServer);
#if defined(_GLIBCXX_HAVE_AT_QUICK_EXIT)
    std::at_quick_exit(&ConnectionServer::removeServer);
#endif
    std::set_terminate(&ConnectionServer::removeServer);
}

ConnectionServer::~ConnectionServer()
{
    killTimer(aliveTimerId);
    removeServer();
}

void ConnectionServer::start()
{
    QLocalServer::removeServer(connectionName);
    localServer.listen(connectionName);
}

void ConnectionServer::setIpcServer(IpcServerInterface *ipcServer)
{
    this->ipcServer = ipcServer;

}

int ConnectionServer::clientProxyCount() const
{
    return ipcClientProxies.size();
}

void ConnectionServer::timerEvent(QTimerEvent *timerEvent)
{
    if (aliveTimerId == timerEvent->timerId())
        sendAliveMessage();
}

void ConnectionServer::handleNewConnection()
{
    QLocalSocket *localSocket(nextPendingConnection());

    ipcClientProxies.emplace_back(ipcServer, localSocket);

    ipcServer->addClient(&ipcClientProxies.back());

    localSockets.push_back(localSocket);

    emit newConnection();
}

void ConnectionServer::sendAliveMessage()
{
    ipcServer->client()->alive();
}

void ConnectionServer::handleSocketDisconnect()
{
    QLocalSocket *senderLocalSocket = static_cast<QLocalSocket*>(sender());

    removeClientProxyWithLocalSocket(senderLocalSocket);
    localSockets.erase(std::remove_if(localSockets.begin(),
                                      localSockets.end(),
                                      [senderLocalSocket](QLocalSocket *localSocketInList) { return localSocketInList == senderLocalSocket;}));

    delayedExitApplicationIfNoSockedIsConnected();
}

void ConnectionServer::removeClientProxyWithLocalSocket(QLocalSocket *localSocket)
{
    ipcClientProxies.erase(std::remove_if(ipcClientProxies.begin(),
                                          ipcClientProxies.end(),
                                          [localSocket](const IpcClientProxy &client) { return client.isUsingThatIoDevice(localSocket);}));
}

QLocalSocket *ConnectionServer::nextPendingConnection()
{
    QLocalSocket *localSocket = localServer.nextPendingConnection();

    connect(localSocket, &QLocalSocket::disconnected, this, &ConnectionServer::handleSocketDisconnect);

    return localSocket;
}

void ConnectionServer::removeServer()
{
    QLocalServer::removeServer(connectionName);
}

void ConnectionServer::delayedExitApplicationIfNoSockedIsConnected()
{
    if (localSockets.size() == 0)
        QTimer::singleShot(60000, this, &ConnectionServer::exitApplicationIfNoSockedIsConnected);
}

void ConnectionServer::exitApplicationIfNoSockedIsConnected()
{
    if (localSockets.size() == 0)
        QCoreApplication::exit();
}

} // namespace ClangBackEnd

