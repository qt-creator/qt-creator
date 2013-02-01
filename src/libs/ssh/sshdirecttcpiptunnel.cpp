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
#include "sshdirecttcpiptunnel.h"
#include "sshdirecttcpiptunnel_p.h"

#include "sshincomingpacket_p.h"
#include "sshsendfacility_p.h"

#include <QTimer>

namespace QSsh {
namespace Internal {

SshDirectTcpIpTunnelPrivate::SshDirectTcpIpTunnelPrivate(quint32 channelId, quint16 remotePort,
        const SshConnectionInfo &connectionInfo, SshSendFacility &sendFacility)
    : AbstractSshChannel(channelId, sendFacility),
      m_remotePort(remotePort),
      m_connectionInfo(connectionInfo)
{
    connect(this, SIGNAL(eof()), SLOT(handleEof()));
}

void SshDirectTcpIpTunnelPrivate::handleChannelSuccess()
{
    throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_MSG_CHANNEL_SUCCESS message.");
}

void SshDirectTcpIpTunnelPrivate::handleChannelFailure()
{
    throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_MSG_CHANNEL_FAILURE message.");
}

void SshDirectTcpIpTunnelPrivate::handleOpenSuccessInternal()
{
    emit initialized();
}

void SshDirectTcpIpTunnelPrivate::handleOpenFailureInternal(const QString &reason)
{
    emit error(reason);
    closeChannel();
}

void SshDirectTcpIpTunnelPrivate::handleChannelDataInternal(const QByteArray &data)
{
    m_data += data;
    emit readyRead();
}

void SshDirectTcpIpTunnelPrivate::handleChannelExtendedDataInternal(quint32 type,
        const QByteArray &data)
{
    qDebug("%s: Unexpected extended channel data. Type is %u, content is '%s'.", Q_FUNC_INFO, type,
           data.constData());
}

void SshDirectTcpIpTunnelPrivate::handleExitStatus(const SshChannelExitStatus &exitStatus)
{
    qDebug("%s: Unexpected exit status %d.", Q_FUNC_INFO, exitStatus.exitStatus);
}

void SshDirectTcpIpTunnelPrivate::handleExitSignal(const SshChannelExitSignal &signal)
{
    qDebug("%s: Unexpected exit signal %s.", Q_FUNC_INFO, signal.signal.constData());
}

void SshDirectTcpIpTunnelPrivate::closeHook()
{
    emit closed();
}

void SshDirectTcpIpTunnelPrivate::handleEof()
{
    /*
     * For some reason, the OpenSSH server only sends EOF when the remote port goes away,
     * but does not close the channel, even though it becomes useless in that case.
     * So we close it ourselves.
     */
    closeChannel();
}

} // namespace Internal

using namespace Internal;

SshDirectTcpIpTunnel::SshDirectTcpIpTunnel(quint32 channelId, quint16 remotePort,
        const SshConnectionInfo &connectionInfo, SshSendFacility &sendFacility)
    : d(new SshDirectTcpIpTunnelPrivate(channelId, remotePort, connectionInfo, sendFacility))
{
    connect(d, SIGNAL(initialized()), SIGNAL(initialized()), Qt::QueuedConnection);
    connect(d, SIGNAL(readyRead()), SIGNAL(readyRead()), Qt::QueuedConnection);
    connect(d, SIGNAL(closed()), SIGNAL(tunnelClosed()), Qt::QueuedConnection);
    connect(d, SIGNAL(error(QString)), SLOT(handleError(QString)), Qt::QueuedConnection);
}

SshDirectTcpIpTunnel::~SshDirectTcpIpTunnel()
{
    d->closeChannel();
    delete d;
}

bool SshDirectTcpIpTunnel::atEnd() const
{
    return QIODevice::atEnd() && d->m_data.isEmpty();
}

qint64 SshDirectTcpIpTunnel::bytesAvailable() const
{
    return QIODevice::bytesAvailable() + d->m_data.count();
}

bool SshDirectTcpIpTunnel::canReadLine() const
{
    return QIODevice::canReadLine() || d->m_data.contains('\n');
}

void SshDirectTcpIpTunnel::close()
{
    d->closeChannel();
    QIODevice::close();
}

void SshDirectTcpIpTunnel::initialize()
{
    QSSH_ASSERT_AND_RETURN(d->channelState() == AbstractSshChannel::Inactive);

    try {
        QIODevice::open(QIODevice::ReadWrite);
        d->m_sendFacility.sendDirectTcpIpPacket(d->localChannelId(), d->initialWindowSize(),
            d->maxPacketSize(), d->m_connectionInfo.peerAddress.toString().toUtf8(),
            d->m_remotePort, d->m_connectionInfo.localAddress.toString().toUtf8(),
            d->m_connectionInfo.localPort);
        d->setChannelState(AbstractSshChannel::SessionRequested);
        d->m_timeoutTimer->start(d->ReplyTimeout);
    }  catch (Botan::Exception &e) { // Won't happen, but let's play it safe.
        qDebug("Botan error: %s", e.what());
        d->closeChannel();
    }
}

qint64 SshDirectTcpIpTunnel::readData(char *data, qint64 maxlen)
{
    const qint64 bytesRead = qMin(qint64(d->m_data.count()), maxlen);
    memcpy(data, d->m_data.constData(), bytesRead);
    d->m_data.remove(0, bytesRead);
    return bytesRead;
}

qint64 SshDirectTcpIpTunnel::writeData(const char *data, qint64 len)
{
    QSSH_ASSERT_AND_RETURN_VALUE(d->channelState() == AbstractSshChannel::SessionEstablished, 0);

    d->sendData(QByteArray(data, len));
    return len;
}

void SshDirectTcpIpTunnel::handleError(const QString &reason)
{
    setErrorString(reason);
    emit error(reason);
}

} // namespace QSsh
