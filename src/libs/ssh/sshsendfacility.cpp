/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "sshsendfacility_p.h"

#include "sshkeyexchange_p.h"
#include "sshoutgoingpacket_p.h"

#include <QTcpSocket>

namespace QSsh {
namespace Internal {

SshSendFacility::SshSendFacility(QTcpSocket *socket)
    : m_clientSeqNr(0), m_socket(socket),
      m_outgoingPacket(m_encrypter, m_clientSeqNr)
{
}

void SshSendFacility::sendPacket()
{
#ifdef CREATOR_SSH_DEBUG
    qDebug("Sending packet, client seq nr is %u", m_clientSeqNr);
#endif
    if (m_socket->isValid()
        && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(m_outgoingPacket.rawData());
        ++m_clientSeqNr;
    }
}

void SshSendFacility::reset()
{
    m_clientSeqNr = 0;
    m_encrypter.clearKeys();
}

void SshSendFacility::recreateKeys(const SshKeyExchange &keyExchange)
{
    m_encrypter.recreateKeys(keyExchange);
}

void SshSendFacility::createAuthenticationKey(const QByteArray &privKeyFileContents)
{
    m_encrypter.createAuthenticationKey(privKeyFileContents);
}

QByteArray SshSendFacility::sendKeyExchangeInitPacket()
{
    const QByteArray &payLoad = m_outgoingPacket.generateKeyExchangeInitPacket();
    sendPacket();
    return payLoad;
}

void SshSendFacility::sendKeyDhInitPacket(const Botan::BigInt &e)
{
    m_outgoingPacket.generateKeyDhInitPacket(e);
    sendPacket();
}

void SshSendFacility::sendKeyEcdhInitPacket(const QByteArray &clientQ)
{
    m_outgoingPacket.generateKeyEcdhInitPacket(clientQ);
    sendPacket();
}

void SshSendFacility::sendNewKeysPacket()
{
    m_outgoingPacket.generateNewKeysPacket();
    sendPacket();
}

void SshSendFacility::sendDisconnectPacket(SshErrorCode reason,
    const QByteArray &reasonString)
{
    m_outgoingPacket.generateDisconnectPacket(reason, reasonString);
    sendPacket();
    }

void SshSendFacility::sendMsgUnimplementedPacket(quint32 serverSeqNr)
{
    m_outgoingPacket.generateMsgUnimplementedPacket(serverSeqNr);
    sendPacket();
}

void SshSendFacility::sendUserAuthServiceRequestPacket()
{
    m_outgoingPacket.generateUserAuthServiceRequestPacket();
    sendPacket();
}

void SshSendFacility::sendUserAuthByPasswordRequestPacket(const QByteArray &user,
    const QByteArray &service, const QByteArray &pwd)
{
    m_outgoingPacket.generateUserAuthByPasswordRequestPacket(user, service, pwd);
    sendPacket();
    }

void SshSendFacility::sendUserAuthByPublicKeyRequestPacket(const QByteArray &user,
    const QByteArray &service)
{
    m_outgoingPacket.generateUserAuthByPublicKeyRequestPacket(user, service);
    sendPacket();
}

void SshSendFacility::sendUserAuthByKeyboardInteractiveRequestPacket(const QByteArray &user,
                                                                     const QByteArray &service)
{
    m_outgoingPacket.generateUserAuthByKeyboardInteractiveRequestPacket(user, service);
    sendPacket();
}

void SshSendFacility::sendUserAuthInfoResponsePacket(const QStringList &responses)
{
    m_outgoingPacket.generateUserAuthInfoResponsePacket(responses);
    sendPacket();
}

void SshSendFacility::sendRequestFailurePacket()
{
    m_outgoingPacket.generateRequestFailurePacket();
    sendPacket();
}

void SshSendFacility::sendIgnorePacket()
{
    m_outgoingPacket.generateIgnorePacket();
    sendPacket();
}

void SshSendFacility::sendInvalidPacket()
{
    m_outgoingPacket.generateInvalidMessagePacket();
    sendPacket();
}

void SshSendFacility::sendSessionPacket(quint32 channelId, quint32 windowSize,
    quint32 maxPacketSize)
{
    m_outgoingPacket.generateSessionPacket(channelId, windowSize,
        maxPacketSize);
    sendPacket();
}

void SshSendFacility::sendDirectTcpIpPacket(quint32 channelId, quint32 windowSize,
    quint32 maxPacketSize, const QByteArray &remoteHost, quint32 remotePort,
    const QByteArray &localIpAddress, quint32 localPort)
{
    m_outgoingPacket.generateDirectTcpIpPacket(channelId, windowSize, maxPacketSize, remoteHost,
            remotePort, localIpAddress, localPort);
    sendPacket();
}

void SshSendFacility::sendPtyRequestPacket(quint32 remoteChannel,
    const SshPseudoTerminal &terminal)
{
    m_outgoingPacket.generatePtyRequestPacket(remoteChannel, terminal);
    sendPacket();
}

void SshSendFacility::sendEnvPacket(quint32 remoteChannel,
    const QByteArray &var, const QByteArray &value)
{
    m_outgoingPacket.generateEnvPacket(remoteChannel, var, value);
    sendPacket();
}

void SshSendFacility::sendExecPacket(quint32 remoteChannel,
    const QByteArray &command)
{
    m_outgoingPacket.generateExecPacket(remoteChannel, command);
    sendPacket();
}

void SshSendFacility::sendShellPacket(quint32 remoteChannel)
{
    m_outgoingPacket.generateShellPacket(remoteChannel);
    sendPacket();
}

void SshSendFacility::sendSftpPacket(quint32 remoteChannel)
{
    m_outgoingPacket.generateSftpPacket(remoteChannel);
    sendPacket();
}

void SshSendFacility::sendWindowAdjustPacket(quint32 remoteChannel,
    quint32 bytesToAdd)
{
    m_outgoingPacket.generateWindowAdjustPacket(remoteChannel, bytesToAdd);
    sendPacket();
}

void SshSendFacility::sendChannelDataPacket(quint32 remoteChannel,
    const QByteArray &data)
{
    m_outgoingPacket.generateChannelDataPacket(remoteChannel, data);
    sendPacket();
}

void SshSendFacility::sendChannelSignalPacket(quint32 remoteChannel,
    const QByteArray &signalName)
{
    m_outgoingPacket.generateChannelSignalPacket(remoteChannel, signalName);
    sendPacket();
}

void SshSendFacility::sendChannelEofPacket(quint32 remoteChannel)
{
    m_outgoingPacket.generateChannelEofPacket(remoteChannel);
    sendPacket();
}

void SshSendFacility::sendChannelClosePacket(quint32 remoteChannel)
{
    m_outgoingPacket.generateChannelClosePacket(remoteChannel);
    sendPacket();
}

} // namespace Internal
} // namespace QSsh
