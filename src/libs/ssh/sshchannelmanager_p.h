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

#ifndef SSHCHANNELLAYER_P_H
#define SSHCHANNELLAYER_P_H

#include <QHash>
#include <QObject>
#include <QSharedPointer>

namespace QSsh {
class SftpChannel;
class SshConnectionInfo;
class SshDirectTcpIpTunnel;
class SshRemoteProcess;

namespace Internal {

class AbstractSshChannel;
class SshIncomingPacket;
class SshSendFacility;

class SshChannelManager : public QObject
{
    Q_OBJECT
public:
    SshChannelManager(SshSendFacility &sendFacility, QObject *parent);

    QSharedPointer<SshRemoteProcess> createRemoteProcess(const QByteArray &command);
    QSharedPointer<SshRemoteProcess> createRemoteShell();
    QSharedPointer<SftpChannel> createSftpChannel();
    QSharedPointer<SshDirectTcpIpTunnel> createTunnel(quint16 remotePort,
            const SshConnectionInfo &connectionInfo);

    int channelCount() const;
    enum CloseAllMode { CloseAllRegular, CloseAllAndReset };
    int closeAllChannels(CloseAllMode mode);

    void handleChannelRequest(const SshIncomingPacket &packet);
    void handleChannelOpen(const SshIncomingPacket &packet);
    void handleChannelOpenFailure(const SshIncomingPacket &packet);
    void handleChannelOpenConfirmation(const SshIncomingPacket &packet);
    void handleChannelSuccess(const SshIncomingPacket &packet);
    void handleChannelFailure(const SshIncomingPacket &packet);
    void handleChannelWindowAdjust(const SshIncomingPacket &packet);
    void handleChannelData(const SshIncomingPacket &packet);
    void handleChannelExtendedData(const SshIncomingPacket &packet);
    void handleChannelEof(const SshIncomingPacket &packet);
    void handleChannelClose(const SshIncomingPacket &packet);

signals:
    void timeout();

private:
    typedef QHash<quint32, AbstractSshChannel *>::Iterator ChannelIterator;

    ChannelIterator lookupChannelAsIterator(quint32 channelId,
        bool allowNotFound = false);
    AbstractSshChannel *lookupChannel(quint32 channelId,
        bool allowNotFound = false);
    void removeChannel(ChannelIterator it);
    void insertChannel(AbstractSshChannel *priv,
        const QSharedPointer<QObject> &pub);

    SshSendFacility &m_sendFacility;
    QHash<quint32, AbstractSshChannel *> m_channels;
    QHash<AbstractSshChannel *, QSharedPointer<QObject> > m_sessions;
    quint32 m_nextLocalChannelId;
};

} // namespace Internal
} // namespace QSsh

#endif // SSHCHANNELLAYER_P_H
