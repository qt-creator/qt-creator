// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
class QmlRelayServer;

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
    virtual int readData(int socketFd, void* buf, size_t size);
    virtual int writeData(int socketFd, const void *data, size_t size);
    virtual void closeConnection();

    IosTool *iosTool() const;
    RelayServer *server() const;
    ServiceSocket m_serverFileDescriptor;
    QTcpSocket *m_clientSocket = nullptr;
    QSocketNotifier *m_serverNotifier = nullptr;
};

class ServiceConnectionRelayer : public Relayer
{
public:
    ServiceConnectionRelayer(RelayServer *parent, QTcpSocket *clientSocket, ServiceConnRef conn);

protected:
    int readData(int socketFd, void* buf, size_t size) override;
    int writeData(int socketFd, const void *data, size_t size) override;
    void closeConnection() override;

private:
    ServiceConnRef m_serviceConn;
};

class RemotePortRelayer: public Relayer
{
    Q_OBJECT

public:
    static const int reconnectMsecDelay = 500;
    static const int maxReconnectAttempts = 2*60*5; // 5 min
    RemotePortRelayer(QmlRelayServer *parent, QTcpSocket *clientSocket);
    void tryRemoteConnect();

signals:
    void didConnect(QmlRelayServer *serv);

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

class GdbRelayServer: public RelayServer
{
    Q_OBJECT

public:
    GdbRelayServer(IosTool *parent, int serverFileDescriptor, ServiceConnRef conn);

protected:
    void newRelayConnection() override;

private:
    int m_serverFileDescriptor = -1;
    ServiceConnRef m_serviceConn = nullptr;
};

class QmlRelayServer: public RelayServer
{
    Q_OBJECT

public:
    QmlRelayServer(IosTool *parent, int remotePort,
                   Ios::DeviceSession *deviceSession);

protected:
    void newRelayConnection() override;

private:
    int m_remotePort;
    DeviceSession *m_deviceSession;
    friend class RemotePortRelayer;
};
}
