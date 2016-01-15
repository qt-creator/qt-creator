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

#ifndef CLANGBACKEND_CONNECTIONSERVER_H
#define CLANGBACKEND_CONNECTIONSERVER_H

#include <ipcclientproxy.h>

#include <QLocalServer>

#include <vector>

namespace ClangBackEnd {

class IpcServerInterface;
class IpcClientProxy;

class CMBIPC_EXPORT ConnectionServer : public QObject
{
    Q_OBJECT
public:
    ConnectionServer(const QString &connectionName);
    ~ConnectionServer();

    void start();
    void setIpcServer(IpcServerInterface *ipcServer);

    int clientProxyCount() const;

    static void removeServer();

signals:
    void newConnection();

protected:
    void timerEvent(QTimerEvent *timerEvent);

private:
    void handleNewConnection();
    void sendAliveMessage();
    void handleSocketDisconnect();
    void removeClientProxyWithLocalSocket(QLocalSocket *localSocket);
    QLocalSocket *nextPendingConnection();
    void delayedExitApplicationIfNoSockedIsConnected();
    void exitApplicationIfNoSockedIsConnected();

private:
    std::vector<IpcClientProxy> ipcClientProxies;
    std::vector<QLocalSocket*> localSockets;
    IpcServerInterface *ipcServer;
    QLocalServer localServer;
    static QString connectionName;
    int aliveTimerId;
};

} // namespace ClangBackEnd

#endif // CLANGBACKEND_CONNECTIONSERVER_H
