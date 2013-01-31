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

#include "sshchannel_p.h"

#include "sshincomingpacket_p.h"
#include "sshsendfacility_p.h"

#include <botan/botan.h>

#include <QTimer>

namespace QSsh {
namespace Internal {

const quint32 MinMaxPacketSize = 32768;
const quint32 NoChannel = 0xffffffffu;

AbstractSshChannel::AbstractSshChannel(quint32 channelId,
    SshSendFacility &sendFacility)
    : m_sendFacility(sendFacility), m_timeoutTimer(new QTimer(this)),
      m_localChannel(channelId), m_remoteChannel(NoChannel),
      m_localWindowSize(initialWindowSize()), m_remoteWindowSize(0),
      m_state(Inactive)
{
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, SIGNAL(timeout()), this, SIGNAL(timeout()));
}

AbstractSshChannel::~AbstractSshChannel()
{

}

void AbstractSshChannel::setChannelState(ChannelState state)
{
    m_state = state;
    if (state == Closed)
        closeHook();
}

void AbstractSshChannel::requestSessionStart()
{
    // Note: We are just being paranoid here about the Botan exceptions,
    // which are extremely unlikely to happen, because if there was a problem
    // with our cryptography stuff, it would have hit us before, on
    // establishing the connection.
    try {
        m_sendFacility.sendSessionPacket(m_localChannel, initialWindowSize(), maxPacketSize());
        setChannelState(SessionRequested);
        m_timeoutTimer->start(ReplyTimeout);
    }  catch (Botan::Exception &e) {
        qDebug("Botan error: %s", e.what());
        closeChannel();
    }
}

void AbstractSshChannel::sendData(const QByteArray &data)
{
    try {
        m_sendBuffer += data;
        flushSendBuffer();
    }  catch (Botan::Exception &e) {
        qDebug("Botan error: %s", e.what());
        closeChannel();
    }
}

quint32 AbstractSshChannel::initialWindowSize()
{
    return maxPacketSize();
}

quint32 AbstractSshChannel::maxPacketSize()
{
    return 16 * 1024 * 1024;
}

void AbstractSshChannel::handleWindowAdjust(quint32 bytesToAdd)
{
    checkChannelActive();

    const quint64 newValue = m_remoteWindowSize + bytesToAdd;
    if (newValue > 0xffffffffu) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Illegal window size requested.");
    }

    m_remoteWindowSize = newValue;
    flushSendBuffer();
}

void AbstractSshChannel::flushSendBuffer()
{
    while (true) {
        const quint32 bytesToSend = qMin(m_remoteMaxPacketSize,
                qMin<quint32>(m_remoteWindowSize, m_sendBuffer.size()));
        if (bytesToSend == 0)
            break;
        const QByteArray &data = m_sendBuffer.left(bytesToSend);
        m_sendFacility.sendChannelDataPacket(m_remoteChannel, data);
        m_sendBuffer.remove(0, bytesToSend);
        m_remoteWindowSize -= bytesToSend;
    }
}

void AbstractSshChannel::handleOpenSuccess(quint32 remoteChannelId,
    quint32 remoteWindowSize, quint32 remoteMaxPacketSize)
{
    if (m_state != SessionRequested) {
       throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
           "Invalid SSH_MSG_CHANNEL_OPEN_CONFIRMATION packet.");
   }
    m_timeoutTimer->stop();

   if (remoteMaxPacketSize < MinMaxPacketSize) {
       throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
           "Maximum packet size too low.");
   }

#ifdef CREATOR_SSH_DEBUG
   qDebug("Channel opened. remote channel id: %u, remote window size: %u, "
       "remote max packet size: %u",
       remoteChannelId, remoteWindowSize, remoteMaxPacketSize);
#endif
   m_remoteChannel = remoteChannelId;
   m_remoteWindowSize = remoteWindowSize;
   m_remoteMaxPacketSize = remoteMaxPacketSize - sizeof(quint32) - sizeof m_remoteChannel - 1;
        // Original value includes packet type, channel number and length field for string.
   setChannelState(SessionEstablished);
   handleOpenSuccessInternal();
}

void AbstractSshChannel::handleOpenFailure(const QString &reason)
{
    if (m_state != SessionRequested) {
       throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
           "Invalid SSH_MSG_CHANNEL_OPEN_FAILURE packet.");
   }
    m_timeoutTimer->stop();

#ifdef CREATOR_SSH_DEBUG
   qDebug("Channel open request failed for channel %u", m_localChannel);
#endif
   handleOpenFailureInternal(reason);
}

void AbstractSshChannel::handleChannelEof()
{
    if (m_state == Inactive || m_state == Closed) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_MSG_CHANNEL_EOF message.");
    }
    m_localWindowSize = 0;
    emit eof();
}

void AbstractSshChannel::handleChannelClose()
{
#ifdef CREATOR_SSH_DEBUG
    qDebug("Receiving CLOSE for channel %u", m_localChannel);
#endif
    if (channelState() == Inactive || channelState() == Closed) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_MSG_CHANNEL_CLOSE message.");
    }
    closeChannel();
    setChannelState(Closed);
}

void AbstractSshChannel::handleChannelData(const QByteArray &data)
{
    const int bytesToDeliver = handleChannelOrExtendedChannelData(data);
    handleChannelDataInternal(bytesToDeliver == data.size()
        ? data : data.left(bytesToDeliver));
}

void AbstractSshChannel::handleChannelExtendedData(quint32 type, const QByteArray &data)
{
    const int bytesToDeliver = handleChannelOrExtendedChannelData(data);
    handleChannelExtendedDataInternal(type, bytesToDeliver == data.size()
        ? data : data.left(bytesToDeliver));
}

void AbstractSshChannel::handleChannelRequest(const SshIncomingPacket &packet)
{
    checkChannelActive();
    const QByteArray &requestType = packet.extractChannelRequestType();
    if (requestType == SshIncomingPacket::ExitStatusType)
        handleExitStatus(packet.extractChannelExitStatus());
    else if (requestType == SshIncomingPacket::ExitSignalType)
        handleExitSignal(packet.extractChannelExitSignal());
    else if (requestType != "eow@openssh.com") // Suppress warning for this one, as it's sent all the time.
        qWarning("Ignoring unknown request type '%s'", requestType.data());
}

int AbstractSshChannel::handleChannelOrExtendedChannelData(const QByteArray &data)
{
    checkChannelActive();

    const int bytesToDeliver = qMin<quint32>(data.size(), maxDataSize());
    if (bytesToDeliver != data.size())
        qWarning("Misbehaving server does not respect local window, clipping.");

    m_localWindowSize -= bytesToDeliver;
    if (m_localWindowSize < maxPacketSize()) {
        m_localWindowSize += maxPacketSize();
        m_sendFacility.sendWindowAdjustPacket(m_remoteChannel, maxPacketSize());
    }
    return bytesToDeliver;
}

void AbstractSshChannel::closeChannel()
{
    if (m_state == CloseRequested) {
        m_timeoutTimer->stop();
    } else if (m_state != Closed) {
        if (m_state == Inactive) {
            setChannelState(Closed);
        } else {
            setChannelState(CloseRequested);
            m_sendFacility.sendChannelEofPacket(m_remoteChannel);
            m_sendFacility.sendChannelClosePacket(m_remoteChannel);
        }
    }
}

void AbstractSshChannel::checkChannelActive()
{
    if (channelState() == Inactive || channelState() == Closed)
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Channel not open.");
}

quint32 AbstractSshChannel::maxDataSize() const
{
    return qMin(m_localWindowSize, maxPacketSize());
}

} // namespace Internal
} // namespace QSsh
