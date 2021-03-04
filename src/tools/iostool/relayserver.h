/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "iostooltypes.h"

#include <QObject>

#include <QTcpSocket>
#include <QSocketNotifier>
#include <QTimer>
#include <QTcpServer>

namespace Ios {
class DeviceSession;
class IosTool;
class RelayServer;
class GenericRelayServer;

class Relayer: public QObject
{
    Q_OBJECT

public:
    Relayer(RelayServer *parent, QTcpSocket *clientSocket);
    ~Relayer();
    void setClientSocket(QTcpSocket *clientSocket);
    bool startRelay(int serverFileDescriptor);

    void handleSocketHasData(int socket);
    void handleClientHasData();
    void handleClientHasError(QAbstractSocket::SocketError error);

protected:
    IosTool *iosTool() const;
    RelayServer *server() const;
    ServiceSocket m_serverFileDescriptor;
    QTcpSocket *m_clientSocket;
    QSocketNotifier *m_serverNotifier;
};

class RemotePortRelayer: public Relayer
{
    Q_OBJECT

public:
    static const int reconnectMsecDelay = 500;
    static const int maxReconnectAttempts = 2*60*5; // 5 min
    RemotePortRelayer(GenericRelayServer *parent, QTcpSocket *clientSocket);
    void tryRemoteConnect();

signals:
    void didConnect(GenericRelayServer *serv);

private:
    QTimer m_remoteConnectTimer;
};

class RelayServer: public QObject
{
    Q_OBJECT

public:
    RelayServer(IosTool *parent);
    ~RelayServer();
    bool startServer();
    void stopServer();
    quint16 serverPort() const;
    IosTool *iosTool() const;

    void handleNewRelayConnection();
    void removeRelayConnection(Relayer *relayer);

protected:
    virtual void newRelayConnection() = 0;

    QTcpServer m_ipv4Server;
    QTcpServer m_ipv6Server;
    quint16 m_port = 0;
    QList<Relayer *> m_connections;
};

class SingleRelayServer: public RelayServer
{
    Q_OBJECT

public:
    SingleRelayServer(IosTool *parent, int serverFileDescriptor);

protected:
    void newRelayConnection() override;

private:
    int m_serverFileDescriptor;
};

class GenericRelayServer: public RelayServer
{
    Q_OBJECT

public:
    GenericRelayServer(IosTool *parent, int remotePort,
                       Ios::DeviceSession *deviceSession);

protected:
    void newRelayConnection() override;

private:
    int m_remotePort;
    DeviceSession *m_deviceSession;
    friend class RemotePortRelayer;
};
}
