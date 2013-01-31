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

#ifndef SSHCONNECTION_P_H
#define SSHCONNECTION_P_H

#include "sshconnection.h"
#include "sshexception_p.h"
#include "sshincomingpacket_p.h"
#include "sshsendfacility_p.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QPair>
#include <QScopedPointer>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QTcpSocket;
QT_END_NAMESPACE

namespace QSsh {
class SftpChannel;
class SshRemoteProcess;
class SshDirectTcpIpTunnel;

namespace Internal {
class SshChannelManager;

// NOTE: When you add stuff here, don't forget to update m_packetHandlers.
enum SshStateInternal {
    SocketUnconnected, // initial and after disconnect
    SocketConnecting, // After connectToHost()
    SocketConnected, // After socket's connected() signal
    UserAuthServiceRequested,
    UserAuthRequested,
    ConnectionEstablished // After service has been started
    // ...
};

enum SshKeyExchangeState {
    NoKeyExchange,
    KexInitSent,
    DhInitSent,
    NewKeysSent,
    KeyExchangeSuccess  // After server's DH_REPLY message
};

class SshConnectionPrivate : public QObject
{
    Q_OBJECT
    friend class QSsh::SshConnection;
public:
    SshConnectionPrivate(SshConnection *conn,
        const SshConnectionParameters &serverInfo);
    ~SshConnectionPrivate();

    void connectToHost();
    void closeConnection(SshErrorCode sshError, SshError userError,
        const QByteArray &serverErrorString, const QString &userErrorString);
    QSharedPointer<SshRemoteProcess> createRemoteProcess(const QByteArray &command);
    QSharedPointer<SshRemoteProcess> createRemoteShell();
    QSharedPointer<SftpChannel> createSftpChannel();
    QSharedPointer<SshDirectTcpIpTunnel> createTunnel(quint16 remotePort);

    SshStateInternal state() const { return m_state; }
    SshError error() const { return m_error; }
    QString errorString() const { return m_errorString; }

signals:
    void connected();
    void disconnected();
    void dataAvailable(const QString &message);
    void error(QSsh::SshError);

private:
    Q_SLOT void handleSocketConnected();
    Q_SLOT void handleIncomingData();
    Q_SLOT void handleSocketError();
    Q_SLOT void handleSocketDisconnected();
    Q_SLOT void handleTimeout();
    Q_SLOT void sendKeepAlivePacket();

    void handleServerId();
    void handlePackets();
    void handleCurrentPacket();
    void handleKeyExchangeInitPacket();
    void handleKeyExchangeReplyPacket();
    void handleNewKeysPacket();
    void handleServiceAcceptPacket();
    void handlePasswordExpiredPacket();
    void handleUserAuthSuccessPacket();
    void handleUserAuthFailurePacket();
    void handleUserAuthBannerPacket();
    void handleGlobalRequest();
    void handleDebugPacket();
    void handleUnimplementedPacket();
    void handleChannelRequest();
    void handleChannelOpen();
    void handleChannelOpenFailure();
    void handleChannelOpenConfirmation();
    void handleChannelSuccess();
    void handleChannelFailure();
    void handleChannelWindowAdjust();
    void handleChannelData();
    void handleChannelExtendedData();
    void handleChannelEof();
    void handleChannelClose();
    void handleDisconnect();
    bool canUseSocket() const;
    void createPrivateKey();

    void sendData(const QByteArray &data);

    typedef void (SshConnectionPrivate::*PacketHandler)();
    typedef QList<SshStateInternal> StateList;
    void setupPacketHandlers();
    void setupPacketHandler(SshPacketType type, const StateList &states,
        PacketHandler handler);

    typedef QPair<StateList, PacketHandler> HandlerInStates;
    QHash<SshPacketType, HandlerInStates> m_packetHandlers;

    static const quint64 InvalidSeqNr;

    QTcpSocket *m_socket;
    SshStateInternal m_state;
    SshKeyExchangeState m_keyExchangeState;
    SshIncomingPacket m_incomingPacket;
    SshSendFacility m_sendFacility;
    SshChannelManager * const m_channelManager;
    const SshConnectionParameters m_connParams;
    QByteArray m_incomingData;
    SshError m_error;
    QString m_errorString;
    QScopedPointer<SshKeyExchange> m_keyExchange;
    QTimer m_timeoutTimer;
    QTimer m_keepAliveTimer;
    bool m_ignoreNextPacket;
    SshConnection *m_conn;
    quint64 m_lastInvalidMsgSeqNr;
    QByteArray m_serverId;
    bool m_serverHasSentDataBeforeId;
};

} // namespace Internal
} // namespace QSsh

#endif // SSHCONNECTION_P_H
