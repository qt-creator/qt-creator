/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

Tunnel::Tunnel(const QSsh::SshConnectionParameters &parameters, QObject *parent)
    : QObject(parent),
      m_connection(new SshConnection(parameters, this)),
      m_tunnelServer(new QTcpServer(this)),
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
    if (!m_tunnelServer->listen(QHostAddress::LocalHost)) {
        std::cerr << "Error opening port: "
                << m_tunnelServer->errorString().toLocal8Bit().constData() << std::endl;
        qApp->exit(EXIT_FAILURE);
        return;
    }
    m_forwardedPort = m_tunnelServer->serverPort();
    connect(m_tunnelServer, SIGNAL(newConnection()), SLOT(handleNewConnection()));

    m_tunnel = m_connection->createTunnel(m_forwardedPort);
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
        m_tunnelSocket->close();
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
    m_tunnelSocket = m_tunnelServer->nextPendingConnection();
    m_tunnelServer->close();
    connect(m_tunnelSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(handleSocketError()));
    connect(m_tunnelSocket, SIGNAL(readyRead()), SLOT(handleClientData()));
    handleClientData();
}

void Tunnel::handleSocketError()
{
    std::cerr << "Socket error: " << m_tunnelSocket->errorString().toLocal8Bit().constData()
            << std::endl;
    qApp->exit(EXIT_FAILURE);
}

void Tunnel::handleClientData()
{
    m_dataReceivedFromClient += m_tunnelSocket->readAll();
    if (m_dataReceivedFromClient == TestData) {
        std::cout << "Client data successfully received by server, now sending data to client..."
                << std::endl;
        m_tunnelSocket->write(ServerDataPrefix + m_dataReceivedFromClient);
    }
}

void Tunnel::handleTimeout()
{
    std::cerr << "Error: Timeout waiting for test completion." << std::endl;
    qApp->exit(EXIT_FAILURE);
}
