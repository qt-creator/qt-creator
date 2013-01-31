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

void SshSendFacility::sendUserAuthByPwdRequestPacket(const QByteArray &user,
    const QByteArray &service, const QByteArray &pwd)
{
    m_outgoingPacket.generateUserAuthByPwdRequestPacket(user, service, pwd);
    sendPacket();
    }

void SshSendFacility::sendUserAuthByKeyRequestPacket(const QByteArray &user,
    const QByteArray &service)
{
    m_outgoingPacket.generateUserAuthByKeyRequestPacket(user, service);
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
