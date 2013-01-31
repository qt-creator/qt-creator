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
#ifndef TUNNEL_H
#define TUNNEL_H

#include <QObject>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QTcpServer;
class QTcpSocket;
QT_END_NAMESPACE

namespace QSsh {
class SshConnection;
class SshConnectionParameters;
class SshDirectTcpIpTunnel;
}

class Tunnel : public QObject
{
    Q_OBJECT
public:
    Tunnel(const QSsh::SshConnectionParameters &parameters, QObject *parent = 0);
    ~Tunnel();

    void run();

private slots:
    void handleConnected();
    void handleConnectionError();
    void handleServerData();
    void handleInitialized();
    void handleTunnelError(const QString &reason);
    void handleTunnelClosed();
    void handleNewConnection();
    void handleSocketError();
    void handleClientData();
    void handleTimeout();

private:
    QSsh::SshConnection * const m_connection;
    QSharedPointer<QSsh::SshDirectTcpIpTunnel> m_tunnel;
    QTcpServer * const m_tunnelServer;
    QTcpSocket *m_tunnelSocket;
    quint16 m_forwardedPort;
    QByteArray m_dataReceivedFromServer;
    QByteArray m_dataReceivedFromClient;
    bool m_expectingChannelClose;
};

#endif // TUNNEL_H
