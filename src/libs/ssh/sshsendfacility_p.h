/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef SSHCONNECTIONOUTSTATE_P_H
#define SSHCONNECTIONOUTSTATE_P_H

#include "sshcryptofacility_p.h"
#include "sshoutgoingpacket_p.h"

QT_BEGIN_NAMESPACE
class QTcpSocket;
QT_END_NAMESPACE


namespace QSsh {
class SshPseudoTerminal;

namespace Internal {
class SshKeyExchange;

class SshSendFacility
{
public:
    SshSendFacility(QTcpSocket *socket);
    void reset();
    void recreateKeys(const SshKeyExchange &keyExchange);
    void createAuthenticationKey(const QByteArray &privKeyFileContents);

    QByteArray sendKeyExchangeInitPacket();
    void sendKeyDhInitPacket(const Botan::BigInt &e);
    void sendNewKeysPacket();
    void sendDisconnectPacket(SshErrorCode reason,
        const QByteArray &reasonString);
    void sendMsgUnimplementedPacket(quint32 serverSeqNr);
    void sendUserAuthServiceRequestPacket();
    void sendUserAuthByPwdRequestPacket(const QByteArray &user,
        const QByteArray &service, const QByteArray &pwd);
    void sendUserAuthByKeyRequestPacket(const QByteArray &user,
        const QByteArray &service);
    void sendRequestFailurePacket();
    void sendIgnorePacket();
    void sendInvalidPacket();
    void sendSessionPacket(quint32 channelId, quint32 windowSize,
        quint32 maxPacketSize);
    void sendPtyRequestPacket(quint32 remoteChannel,
        const SshPseudoTerminal &terminal);
    void sendEnvPacket(quint32 remoteChannel, const QByteArray &var,
        const QByteArray &value);
    void sendExecPacket(quint32 remoteChannel, const QByteArray &command);
    void sendShellPacket(quint32 remoteChannel);
    void sendSftpPacket(quint32 remoteChannel);
    void sendWindowAdjustPacket(quint32 remoteChannel, quint32 bytesToAdd);
    void sendChannelDataPacket(quint32 remoteChannel, const QByteArray &data);
    void sendChannelSignalPacket(quint32 remoteChannel,
        const QByteArray &signalName);
    void sendChannelEofPacket(quint32 remoteChannel);
    void sendChannelClosePacket(quint32 remoteChannel);
    quint32 nextClientSeqNr() const { return m_clientSeqNr; }

private:
    void sendPacket();

    quint32 m_clientSeqNr;
    SshEncryptionFacility m_encrypter;
    QTcpSocket *m_socket;
    SshOutgoingPacket m_outgoingPacket;
};

} // namespace Internal
} // namespace QSsh

#endif // SSHCONNECTIONOUTSTATE_P_H
