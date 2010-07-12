/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef SSHCONNECTIONOUTSTATE_P_H
#define SSHCONNECTIONOUTSTATE_P_H

#include "sshcryptofacility_p.h"
#include "sshoutgoingpacket_p.h"

QT_BEGIN_NAMESPACE
class QTcpSocket;
QT_END_NAMESPACE


namespace Core {
namespace Internal {
class SshKeyExchange;

class SshSendFacility
{
public:
    SshSendFacility(QTcpSocket *socket);
    void reset();
    void recreateKeys(const SshKeyExchange &keyExchange);
    void createAuthenticationKey(const QByteArray &privKeyFileContents);

    SshOutgoingPacket::Payload sendKeyExchangeInitPacket();
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
    void sendSessionPacket(quint32 channelId, quint32 windowSize,
        quint32 maxPacketSize);
    void sendEnvPacket(quint32 remoteChannel, const QByteArray &var,
        const QByteArray &value);
    void sendExecPacket(quint32 remoteChannel, const QByteArray &command);
    void sendSftpPacket(quint32 remoteChannel);
    void sendWindowAdjustPacket(quint32 remoteChannel, quint32 bytesToAdd);
    void sendChannelDataPacket(quint32 remoteChannel, const QByteArray &data);
    void sendChannelSignalPacket(quint32 remoteChannel,
        const QByteArray &signalName);
    void sendChannelEofPacket(quint32 remoteChannel);
    void sendChannelClosePacket(quint32 remoteChannel);

private:
    void sendPacket();

    quint32 m_clientSeqNr;
    SshEncryptionFacility m_encrypter;
    QTcpSocket *m_socket;
    SshOutgoingPacket m_outgoingPacket;
};

} // namespace Internal
} // namespace Core

#endif // SSHCONNECTIONOUTSTATE_P_H
