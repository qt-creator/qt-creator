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

#include "tunnel.h"

#include <ssh/sshconnection.h>
#include <ssh/sshdirecttcpiptunnel.h>

#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include <cstdlib>
#include <iostream>

const QByteArray ServerDataPrefix("Received the following data: ");
const QByteArray TestData("Urgsblubb?");

using namespace QSsh;

Tunnel::Tunnel(const SshConnectionParameters &parameters, QObject *parent)
    : QObject(parent),
      m_connection(new SshConnection(parameters, this)),
      m_targetServer(new QTcpServer(this)),
      m_expectingChannelClose(false)
{
    connect(m_connection, SIGNAL(connected()), SLOT(handleConnected()));
    connect(m_connection, SIGNAL(error(QSsh::SshError)), SLOT(handleConnectionError()));
}

Tunnel::~Tunnel()
{
}

void Tunnel::run()
{
    std::cout << "Connecting to SSH server..." << std::endl;
    m_connection->connectToHost();
}

void Tunnel::handleConnectionError()
{
    std::cerr << "SSH connection error: " << qPrintable(m_connection->errorString()) << std::endl;
    qApp->exit(EXIT_FAILURE);
}

void Tunnel::handleConnected()
{
    std::cout << "Opening server side..." << std::endl;
    if (!m_targetServer->listen(QHostAddress::LocalHost)) {
        std::cerr << "Error opening port: "
                << m_targetServer->errorString().toLocal8Bit().constData() << std::endl;
        qApp->exit(EXIT_FAILURE);
        return;
    }
    m_targetPort = m_targetServer->serverPort();
    connect(m_targetServer, SIGNAL(newConnection()), SLOT(handleNewConnection()));

    m_tunnel = m_connection->createTunnel(QLatin1String("localhost"), 1024, // made-up values
                                          QLatin1String("localhost"), m_targetPort);
    connect(m_tunnel.data(), SIGNAL(initialized()), SLOT(handleInitialized()));
    connect(m_tunnel.data(), SIGNAL(error(QString)), SLOT(handleTunnelError(QString)));
    connect(m_tunnel.data(), SIGNAL(readyRead()), SLOT(handleServerData()));
    connect(m_tunnel.data(), SIGNAL(tunnelClosed()), SLOT(handleTunnelClosed()));

    std::cout << "Initializing tunnel..." << std::endl;
    m_tunnel->initialize();
}

void Tunnel::handleInitialized()
{
    std::cout << "Writing data into the tunnel..." << std::endl;
    m_tunnel->write(TestData);
    QTimer * const timeoutTimer = new QTimer(this);
    connect(timeoutTimer, SIGNAL(timeout()), SLOT(handleTimeout()));
    timeoutTimer->start(10000);
}

void Tunnel::handleServerData()
{
    m_dataReceivedFromServer += m_tunnel->readAll();
    if (m_dataReceivedFromServer == ServerDataPrefix + TestData) {
        std::cout << "Data exchange successful. Closing server socket..." << std::endl;
        m_expectingChannelClose = true;
        m_targetSocket->close();
    }
}

void Tunnel::handleTunnelError(const QString &reason)
{
    std::cerr << "Tunnel error: " << reason.toLocal8Bit().constData() << std::endl;
    qApp->exit(EXIT_FAILURE);
}

void Tunnel::handleTunnelClosed()
{
    if (m_expectingChannelClose) {
        std::cout << "Successfully detected channel close." << std::endl;
        std::cout << "Test finished successfully." << std::endl;
        qApp->quit();
    } else {
        std::cerr << "Error: Remote host closed channel." << std::endl;
        qApp->exit(EXIT_FAILURE);
    }
}

void Tunnel::handleNewConnection()
{
    m_targetSocket = m_targetServer->nextPendingConnection();
    m_targetServer->close();
    connect(m_targetSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(handleSocketError()));
    connect(m_targetSocket, SIGNAL(readyRead()), SLOT(handleClientData()));
    handleClientData();
}

void Tunnel::handleSocketError()
{
    std::cerr << "Socket error: " << m_targetSocket->errorString().toLocal8Bit().constData()
            << std::endl;
    qApp->exit(EXIT_FAILURE);
}

void Tunnel::handleClientData()
{
    m_dataReceivedFromClient += m_targetSocket->readAll();
    if (m_dataReceivedFromClient == TestData) {
        std::cout << "Client data successfully received by server, now sending data to client..."
                << std::endl;
        m_targetSocket->write(ServerDataPrefix + m_dataReceivedFromClient);
    }
}

void Tunnel::handleTimeout()
{
    std::cerr << "Error: Timeout waiting for test completion." << std::endl;
    qApp->exit(EXIT_FAILURE);
}
