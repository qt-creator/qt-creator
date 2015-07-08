/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    void sendAliveCommand();
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
