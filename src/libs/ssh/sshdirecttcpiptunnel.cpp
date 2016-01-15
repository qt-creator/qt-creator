/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "sshdirecttcpiptunnel.h"
#include "sshdirecttcpiptunnel_p.h"

#include "sshincomingpacket_p.h"
#include "sshlogging_p.h"
#include "sshsendfacility_p.h"

#include <QTimer>

namespace QSsh {
namespace Internal {

SshDirectTcpIpTunnelPrivate::SshDirectTcpIpTunnelPrivate(quint32 channelId,
        const QString &originatingHost, quint16 originatingPort, const QString &remoteHost,
        quint16 remotePort, SshSendFacility &sendFacility)
    : AbstractSshChannel(channelId, sendFacility),
      m_originatingHost(originatingHost),
      m_originatingPort(originatingPort),
      m_remoteHost(remoteHost),
      m_remotePort(remotePort)
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
    qCWarning(sshLog, "%s: Unexpected extended channel data. Type is %u, content is '%s'.",
              Q_FUNC_INFO, type, data.constData());
}

void SshDirectTcpIpTunnelPrivate::handleExitStatus(const SshChannelExitStatus &exitStatus)
{
    qCWarning(sshLog, "%s: Unexpected exit status %d.", Q_FUNC_INFO, exitStatus.exitStatus);
}

void SshDirectTcpIpTunnelPrivate::handleExitSignal(const SshChannelExitSignal &signal)
{
    qCWarning(sshLog, "%s: Unexpected exit signal %s.", Q_FUNC_INFO, signal.signal.constData());
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

SshDirectTcpIpTunnel::SshDirectTcpIpTunnel(quint32 channelId, const QString &originatingHost,
        quint16 originatingPort, const QString &remoteHost, quint16 remotePort,
        SshSendFacility &sendFacility)
    : d(new SshDirectTcpIpTunnelPrivate(channelId, originatingHost, originatingPort, remoteHost,
                                        remotePort, sendFacility))
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
            d->maxPacketSize(), d->m_remoteHost.toUtf8(), d->m_remotePort,
            d->m_originatingHost.toUtf8(), d->m_originatingPort);
        d->setChannelState(AbstractSshChannel::SessionRequested);
        d->m_timeoutTimer.start(d->ReplyTimeout);
    }  catch (const Botan::Exception &e) { // Won't happen, but let's play it safe.
        qCWarning(sshLog, "Botan error: %s", e.what());
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
