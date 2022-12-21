// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "relayserver.h"

#include "iostool.h"
#include "mobiledevicelib.h"

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#endif

// avoid utils dependency
#define QTC_CHECK(cond) if (cond) {} else { qWarning() << "assert failed " << #cond << " " \
    << __FILE__ << ":" << __LINE__; } do {} while (0)

namespace Ios {
Relayer::Relayer(RelayServer *parent, QTcpSocket *clientSocket) :
    QObject(parent), m_serverFileDescriptor(0), m_clientSocket(0), m_serverNotifier(0)
{
    setClientSocket(clientSocket);
}

Relayer::~Relayer()
{
    closeConnection();
}

void Relayer::setClientSocket(QTcpSocket *clientSocket)
{
    QTC_CHECK(!m_clientSocket);
    m_clientSocket = clientSocket;
    if (m_clientSocket) {
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        const auto errorOccurred = QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error);
#else
        const auto errorOccurred = &QAbstractSocket::errorOccurred;
#endif
        connect(m_clientSocket, errorOccurred, this, &Relayer::handleClientHasError);
        connect(m_clientSocket, &QAbstractSocket::disconnected,
                this, [this](){server()->removeRelayConnection(this);});
    }
}

bool Relayer::startRelay(int serverFileDescriptor)
{
    QTC_CHECK(!m_serverFileDescriptor);
    m_serverFileDescriptor = serverFileDescriptor;
    if (!m_clientSocket || m_serverFileDescriptor <= 0)
        return false;
    fcntl(serverFileDescriptor,F_SETFL, fcntl(serverFileDescriptor, F_GETFL) | O_NONBLOCK);
    connect(m_clientSocket, &QIODevice::readyRead, this, &Relayer::handleClientHasData);
    m_serverNotifier = new QSocketNotifier(m_serverFileDescriptor, QSocketNotifier::Read, this);
    connect(m_serverNotifier, &QSocketNotifier::activated, this, &Relayer::handleSocketHasData);
    // no way to check if an error did happen?
    if (m_clientSocket->bytesAvailable() > 0)
        handleClientHasData();
    return true;
}

void Relayer::handleSocketHasData(int socket)
{
    m_serverNotifier->setEnabled(false);
    char buf[255];
    while (true) {
        qptrdiff rRead = readData(socket, &buf, sizeof(buf)-1);
        if (rRead == -1) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN) {
                m_serverNotifier->setEnabled(true);
                return;
            }
            iosTool()->errorMsg(qt_error_string(errno));
            close(socket);
            iosTool()->stopRelayServers(-1);
            return;
        }
        if (rRead == 0) {
            iosTool()->stopRelayServers(0);
            return;
        }
        if (iosTool()->echoRelays()) {
            iosTool()->writeMaybeBin(QString::fromLatin1("%1 serverReplies:")
                                     .arg((quintptr)(void *)this), buf, rRead);
        }
        qint64 pos = 0;
        while (true) {
            qint64 writtenNow = m_clientSocket->write(buf + int(pos), rRead);
            if (writtenNow == -1) {
                iosTool()->writeMsg(m_clientSocket->errorString());
                iosTool()->stopRelayServers(-1);
                return;
            }
            if (writtenNow < rRead) {
                pos += writtenNow;
                rRead -= qptrdiff(writtenNow);
            } else {
                break;
            }
        }
        m_clientSocket->flush();
    }
}

void Relayer::handleClientHasData()
{
    char buf[255];
    while (true) {
        qint64 toRead = m_clientSocket->bytesAvailable();
        if (qint64(sizeof(buf)-1) < toRead)
            toRead = sizeof(buf)-1;
        qint64 rRead = m_clientSocket->read(buf, toRead);
        if (rRead == -1) {
            iosTool()->errorMsg(m_clientSocket->errorString());
            iosTool()->stopRelayServers();
            return;
        }
        if (rRead == 0) {
            if (!m_clientSocket->isOpen())
                iosTool()->stopRelayServers();
            return;
        }
        int pos = 0;
        int irep = 0;
        if (iosTool()->echoRelays()) {
            iosTool()->writeMaybeBin(QString::fromLatin1("%1 clientAsks:")
                                     .arg((quintptr)(void *)this), buf, rRead);
        }
        while (true) {
            qptrdiff written = writeData(m_serverFileDescriptor, buf + pos, rRead);
            if (written == -1) {
                if (errno == EINTR)
                    continue;
                if (errno == EAGAIN) {
                    if (++irep > 10) {
                        sleep(1);
                        irep = 0;
                    }
                    continue;
                }
                iosTool()->errorMsg(qt_error_string(errno));
                iosTool()->stopRelayServers();
                return;
            }
            if (written == 0) {
                iosTool()->stopRelayServers();
                return;
            }
            if (written < rRead) {
                pos += written;
                rRead -= written;
            } else {
                break;
            }
        }
    }
}

void Relayer::handleClientHasError(QAbstractSocket::SocketError error)
{
    iosTool()->errorMsg(tr("iOS Debugging connection to creator failed with error %1").arg(error));
    server()->removeRelayConnection(this);
}

int Relayer::readData(int socketFd, void *buf, size_t size)
{
    return read(socketFd, buf, size);
}

int Relayer::writeData(int socketFd, const void *data, size_t size)
{
    return write(socketFd, data, size);
}

void Relayer::closeConnection()
{
    if (m_serverFileDescriptor > 0) {
        ::close(m_serverFileDescriptor);
        m_serverFileDescriptor = -1;
        if (m_serverNotifier) {
            delete m_serverNotifier;
            m_serverNotifier = nullptr;
        }
    }
    if (m_clientSocket->isOpen())
        m_clientSocket->close();
    delete m_clientSocket;
    m_clientSocket = nullptr;
}

IosTool *Relayer::iosTool() const
{
    return (server() ? server()->iosTool() : 0);
}

RelayServer *Relayer::server() const
{
    return qobject_cast<RelayServer *>(parent());
}

RemotePortRelayer::RemotePortRelayer(QmlRelayServer *parent, QTcpSocket *clientSocket) :
    Relayer(parent, clientSocket)
{
    m_remoteConnectTimer.setSingleShot(true);
    m_remoteConnectTimer.setInterval(reconnectMsecDelay);
    connect(&m_remoteConnectTimer, &QTimer::timeout, this, &RemotePortRelayer::tryRemoteConnect);
}

void RemotePortRelayer::tryRemoteConnect()
{
    iosTool()->errorMsg(QLatin1String("tryRemoteConnect"));
    if (m_serverFileDescriptor > 0)
        return;
    ServiceSocket ss;
    QmlRelayServer *grServer = qobject_cast<QmlRelayServer *>(server());
    if (!grServer)
        return;
    if (grServer->m_deviceSession->connectToPort(grServer->m_remotePort, &ss)) {
        if (ss > 0) {
            iosTool()->errorMsg(QString::fromLatin1("tryRemoteConnect *succeeded* on remote port %1")
                                .arg(grServer->m_remotePort));
            startRelay(ss);
            emit didConnect(grServer);
            return;
        }
    }
    iosTool()->errorMsg(QString::fromLatin1("tryRemoteConnect *failed* on remote port %1")
                        .arg(grServer->m_remotePort));
    m_remoteConnectTimer.start();
}

RelayServer::RelayServer(IosTool *parent):
    QObject(parent)
{ }

RelayServer::~RelayServer()
{
    stopServer();
}

bool RelayServer::startServer()
{
    QTC_CHECK(!m_ipv4Server.isListening());
    QTC_CHECK(!m_ipv6Server.isListening());

    connect(&m_ipv4Server, &QTcpServer::newConnection,
            this, &RelayServer::handleNewRelayConnection);
    connect(&m_ipv6Server, &QTcpServer::newConnection,
            this, &RelayServer::handleNewRelayConnection);

    m_port = 0;
    if (m_ipv4Server.listen(QHostAddress(QHostAddress::LocalHost), 0))
        m_port = m_ipv4Server.serverPort();
    if (m_ipv6Server.listen(QHostAddress(QHostAddress::LocalHostIPv6), m_port))
        m_port = m_ipv6Server.serverPort();

    return m_port > 0;
}

void RelayServer::stopServer()
{
    qDeleteAll(m_connections);
    if (m_ipv4Server.isListening())
        m_ipv4Server.close();
    if (m_ipv6Server.isListening())
        m_ipv6Server.close();
}

quint16 RelayServer::serverPort() const
{
    return m_port;
}

IosTool *RelayServer::iosTool() const
{
    return qobject_cast<IosTool *>(parent());
}

void RelayServer::handleNewRelayConnection()
{
    iosTool()->errorMsg(QLatin1String("handleNewRelayConnection"));
    newRelayConnection();
}

void RelayServer::removeRelayConnection(Relayer *relayer)
{
    m_connections.removeAll(relayer);
    relayer->deleteLater();
}

GdbRelayServer::GdbRelayServer(IosTool *parent,
                                     int serverFileDescriptor, ServiceConnRef conn) :
    RelayServer(parent),
    m_serverFileDescriptor(serverFileDescriptor),
    m_serviceConn(conn)
{
    if (m_serverFileDescriptor > 0)
        fcntl(m_serverFileDescriptor, F_SETFL, fcntl(m_serverFileDescriptor, F_GETFL, 0) | O_NONBLOCK);
}

void GdbRelayServer::newRelayConnection()
{
    QTcpSocket *clientSocket = m_ipv4Server.hasPendingConnections()
            ? m_ipv4Server.nextPendingConnection() : m_ipv6Server.nextPendingConnection();
    if (m_connections.size() > 0) {
        delete clientSocket;
        return;
    }
    if (clientSocket) {
        Relayer *newConnection = new ServiceConnectionRelayer(this, clientSocket, m_serviceConn);
        m_connections.append(newConnection);
        newConnection->startRelay(m_serverFileDescriptor);
    }
}

QmlRelayServer::QmlRelayServer(IosTool *parent, int remotePort,
                                       Ios::DeviceSession *deviceSession) :
    RelayServer(parent),
    m_remotePort(remotePort),
    m_deviceSession(deviceSession)
{
    parent->errorMsg(QLatin1String("created qml server"));
}


void QmlRelayServer::newRelayConnection()
{
    QTcpSocket *clientSocket = m_ipv4Server.hasPendingConnections()
            ? m_ipv4Server.nextPendingConnection() : m_ipv6Server.nextPendingConnection();
    if (clientSocket) {
        iosTool()->errorMsg(QString::fromLatin1("setting up relayer for new connection"));
        RemotePortRelayer *newConnection = new RemotePortRelayer(this, clientSocket);
        m_connections.append(newConnection);
        newConnection->tryRemoteConnect();
    }
}

ServiceConnectionRelayer::ServiceConnectionRelayer(RelayServer *parent, QTcpSocket *clientSocket,
                                                   ServiceConnRef conn)
    : Relayer(parent, clientSocket),
      m_serviceConn(conn)
{

}

int ServiceConnectionRelayer::readData(int socketFd, void *buf, size_t size)
{
    Q_UNUSED(socketFd)
    if (!buf || !m_serviceConn)
        return 0;
    MobileDeviceLib &mlib = MobileDeviceLib::instance();
    return mlib.serviceConnectionReceive(m_serviceConn, buf, size);
}

int ServiceConnectionRelayer::writeData(int socketFd, const void *data, size_t size)
{
    Q_UNUSED(socketFd)
    if (!data || !m_serviceConn)
        return 0;
    MobileDeviceLib &mLib = MobileDeviceLib::instance();
    return mLib.serviceConnectionSend(m_serviceConn, data, size);
}

void ServiceConnectionRelayer::closeConnection()
{
    Relayer::closeConnection();
    if (m_serviceConn) {
        MobileDeviceLib &mLib = MobileDeviceLib::instance();
        mLib.serviceConnectionInvalidate(m_serviceConn);
        m_serviceConn = nullptr;
    }

}

}
