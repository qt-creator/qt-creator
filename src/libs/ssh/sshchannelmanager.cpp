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

#include "sshchannelmanager_p.h"

#include "sftpchannel.h"
#include "sftpchannel_p.h"
#include "sshdirecttcpiptunnel.h"
#include "sshdirecttcpiptunnel_p.h"
#include "sshincomingpacket_p.h"
#include "sshremoteprocess.h"
#include "sshremoteprocess_p.h"
#include "sshsendfacility_p.h"

#include <QList>

namespace QSsh {
namespace Internal {

SshChannelManager::SshChannelManager(SshSendFacility &sendFacility,
    QObject *parent)
    : QObject(parent), m_sendFacility(sendFacility), m_nextLocalChannelId(0)
{
}

void SshChannelManager::handleChannelRequest(const SshIncomingPacket &packet)
{
    lookupChannel(packet.extractRecipientChannel())
        ->handleChannelRequest(packet);
}

void SshChannelManager::handleChannelOpen(const SshIncomingPacket &)
{
    throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
        "Server tried to open channel on client.");
}

void SshChannelManager::handleChannelOpenFailure(const SshIncomingPacket &packet)
{
   const SshChannelOpenFailure &failure = packet.extractChannelOpenFailure();
   ChannelIterator it = lookupChannelAsIterator(failure.localChannel);
   try {
       it.value()->handleOpenFailure(failure.reasonString);
   } catch (SshServerException &e) {
       removeChannel(it);
       throw e;
   }
   removeChannel(it);
}

void SshChannelManager::handleChannelOpenConfirmation(const SshIncomingPacket &packet)
{
   const SshChannelOpenConfirmation &confirmation
       = packet.extractChannelOpenConfirmation();
   lookupChannel(confirmation.localChannel)->handleOpenSuccess(confirmation.remoteChannel,
       confirmation.remoteWindowSize, confirmation.remoteMaxPacketSize);
}

void SshChannelManager::handleChannelSuccess(const SshIncomingPacket &packet)
{
    lookupChannel(packet.extractRecipientChannel())->handleChannelSuccess();
}

void SshChannelManager::handleChannelFailure(const SshIncomingPacket &packet)
{
    lookupChannel(packet.extractRecipientChannel())->handleChannelFailure();
}

void SshChannelManager::handleChannelWindowAdjust(const SshIncomingPacket &packet)
{
    const SshChannelWindowAdjust adjust = packet.extractWindowAdjust();
    lookupChannel(adjust.localChannel)->handleWindowAdjust(adjust.bytesToAdd);
}

void SshChannelManager::handleChannelData(const SshIncomingPacket &packet)
{
    const SshChannelData &data = packet.extractChannelData();
    lookupChannel(data.localChannel)->handleChannelData(data.data);
}

void SshChannelManager::handleChannelExtendedData(const SshIncomingPacket &packet)
{
    const SshChannelExtendedData &data = packet.extractChannelExtendedData();
    lookupChannel(data.localChannel)->handleChannelExtendedData(data.type, data.data);
}

void SshChannelManager::handleChannelEof(const SshIncomingPacket &packet)
{
    AbstractSshChannel * const channel
        = lookupChannel(packet.extractRecipientChannel(), true);
    if (channel)
        channel->handleChannelEof();
}

void SshChannelManager::handleChannelClose(const SshIncomingPacket &packet)
{
    const quint32 channelId = packet.extractRecipientChannel();

    ChannelIterator it = lookupChannelAsIterator(channelId, true);
    if (it != m_channels.end()) {
        it.value()->handleChannelClose();
        removeChannel(it);
    }
}

SshChannelManager::ChannelIterator SshChannelManager::lookupChannelAsIterator(quint32 channelId,
    bool allowNotFound)
{
    ChannelIterator it = m_channels.find(channelId);
    if (it == m_channels.end() && !allowNotFound) {
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid channel id.",
            tr("Invalid channel id %1").arg(channelId));
    }
    return it;
}

AbstractSshChannel *SshChannelManager::lookupChannel(quint32 channelId,
    bool allowNotFound)
{
    ChannelIterator it = lookupChannelAsIterator(channelId, allowNotFound);
    return it == m_channels.end() ? 0 : it.value();
}

QSsh::SshRemoteProcess::Ptr SshChannelManager::createRemoteProcess(const QByteArray &command)
{
    SshRemoteProcess::Ptr proc(new SshRemoteProcess(command, m_nextLocalChannelId++, m_sendFacility));
    insertChannel(proc->d, proc);
    return proc;
}

QSsh::SshRemoteProcess::Ptr SshChannelManager::createRemoteShell()
{
    SshRemoteProcess::Ptr proc(new SshRemoteProcess(m_nextLocalChannelId++, m_sendFacility));
    insertChannel(proc->d, proc);
    return proc;
}

QSsh::SftpChannel::Ptr SshChannelManager::createSftpChannel()
{
    SftpChannel::Ptr sftp(new SftpChannel(m_nextLocalChannelId++, m_sendFacility));
    insertChannel(sftp->d, sftp);
    return sftp;
}

SshDirectTcpIpTunnel::Ptr SshChannelManager::createTunnel(quint16 remotePort,
        const SshConnectionInfo &connectionInfo)
{
    SshDirectTcpIpTunnel::Ptr tunnel(new SshDirectTcpIpTunnel(m_nextLocalChannelId++, remotePort,
            connectionInfo, m_sendFacility));
    insertChannel(tunnel->d, tunnel);
    return tunnel;
}

void SshChannelManager::insertChannel(AbstractSshChannel *priv,
    const QSharedPointer<QObject> &pub)
{
    connect(priv, SIGNAL(timeout()), this, SIGNAL(timeout()));
    m_channels.insert(priv->localChannelId(), priv);
    m_sessions.insert(priv, pub);
}

int SshChannelManager::closeAllChannels(CloseAllMode mode)
{
    int count = 0;
    for (ChannelIterator it = m_channels.begin(); it != m_channels.end(); ++it) {
        AbstractSshChannel * const channel = it.value();
        QSSH_ASSERT(channel->channelState() != AbstractSshChannel::Closed);
        if (channel->channelState() != AbstractSshChannel::CloseRequested) {
            channel->closeChannel();
            ++count;
        }
    }
    if (mode == CloseAllAndReset) {
        m_channels.clear();
        m_sessions.clear();
    }
    return count;
}

int SshChannelManager::channelCount() const
{
    return m_channels.count();
}

void SshChannelManager::removeChannel(ChannelIterator it)
{
    Q_ASSERT(it != m_channels.end() && "Unexpected channel lookup failure.");
    const int removeCount = m_sessions.remove(it.value());
    Q_ASSERT(removeCount == 1 && "Session for channel not found.");
    m_channels.erase(it);
}

} // namespace Internal
} // namespace QSsh
