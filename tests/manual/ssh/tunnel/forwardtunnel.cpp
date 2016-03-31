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

#include "forwardtunnel.h"

#include <ssh/sshconnection.h>
#include <ssh/sshtcpipforwardserver.h>

#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>

#include <iostream>

const QByteArray ClientDataPrefix("Received the following data: ");
const QByteArray TestData("Urgsblubb?");

ForwardTunnel::ForwardTunnel(const QSsh::SshConnectionParameters &parameters, QObject *parent)
    : QObject(parent),
      m_connection(new QSsh::SshConnection(parameters, this)),
      m_targetSocket(0),
      m_targetPort(0)
{
    connect(m_connection, &QSsh::SshConnection::connected, this, &ForwardTunnel::handleConnected);
    connect(m_connection, &QSsh::SshConnection::error, this, &ForwardTunnel::handleConnectionError);
}

void ForwardTunnel::run()
{
    std::cout << "Connecting to SSH server..." << std::endl;
    m_connection->connectToHost();
}

void ForwardTunnel::handleConnected()
{
    {
        QTcpServer server;
        if (server.listen(QHostAddress::LocalHost)) {
            m_targetPort = server.serverPort();
        } else {
            std::cerr << "Error while searching for free port: "
                      << server.errorString().toLocal8Bit().constData() << std::endl;
            emit finished(EXIT_FAILURE);
        }
    }

    std::cout << "Initializing tunnel..." << std::endl;
    m_server = m_connection->createForwardServer(QLatin1String("localhost"), m_targetPort);
    connect(m_server.data(), &QSsh::SshTcpIpForwardServer::newConnection,
            this, &ForwardTunnel::handleNewConnection);
    connect(m_server.data(), &QSsh::SshTcpIpForwardServer::stateChanged,
            this, [this](QSsh::SshTcpIpForwardServer::State state) {
        if (state == QSsh::SshTcpIpForwardServer::Listening)
            handleInitialized();
        else if (state == QSsh::SshTcpIpForwardServer::Inactive)
            handleServerClosed();
    });
    connect(m_server.data(), &QSsh::SshTcpIpForwardServer::error,
            this, &ForwardTunnel::handleServerError);

    m_server->initialize();
}

void ForwardTunnel::handleConnectionError(QSsh::SshError error)
{
    std::cout << "SSH connection error: " << error << " " << qPrintable(m_connection->errorString())
              << std::endl;
    emit finished(EXIT_FAILURE);
}

void ForwardTunnel::handleInitialized()
{
    std::cout << "Forward tunnel initialized, connecting ..." << std::endl;
    m_targetSocket = new QTcpSocket(this);
    connect(m_targetSocket, &QTcpSocket::connected, this, [this](){
        m_targetSocket->write(ClientDataPrefix + TestData);
    });

    connect(m_targetSocket, &QTcpSocket::readyRead, this, [this](){
        m_dataReceivedFromServer += m_targetSocket->readAll();
        if (m_dataReceivedFromServer == ClientDataPrefix + TestData) {
            std::cout << "Data exchange successful. Closing client socket..." << std::endl;
            m_targetSocket->close();
        }
    });
    m_targetSocket->connectToHost(QLatin1String("localhost"), m_targetPort);
}

void ForwardTunnel::handleServerError(const QString &reason)
{
    std::cerr << "Tunnel error:" << reason.toUtf8().constData() << std::endl;
    emit finished(EXIT_FAILURE);
}

void ForwardTunnel::handleServerClosed()
{
    std::cout << "Forward tunnel closed" << std::endl;
    emit finished(EXIT_SUCCESS);
}

void ForwardTunnel::handleNewConnection()
{
    std::cout << "Connection established" << std::endl;
    QSsh::SshForwardedTcpIpTunnel::Ptr tunnel = m_server->nextPendingConnection();

    connect(tunnel.data(), &QIODevice::readyRead, this, [this, tunnel](){
        m_dataReceivedFromClient += tunnel->readAll();
        if (m_dataReceivedFromClient == ClientDataPrefix + TestData) {
            std::cout << "Data received successful. Sending it back..." << std::endl;
            tunnel->write(m_dataReceivedFromClient);
        }
    });

    connect(tunnel.data(), &QIODevice::aboutToClose, this, [this](){
        std::cout << "Server Connection closed, closing tunnel" << std::endl;
        m_server->close();
    });
}

void ForwardTunnel::handleSocketError()
{
    std::cerr << "Socket error" << std::endl;
    emit finished(EXIT_FAILURE);
}
