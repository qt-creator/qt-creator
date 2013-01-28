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

#include "sshincomingpacket_p.h"

#include "sshcapabilities_p.h"

namespace QSsh {
namespace Internal {

const QByteArray SshIncomingPacket::ExitStatusType("exit-status");
const QByteArray SshIncomingPacket::ExitSignalType("exit-signal");

SshIncomingPacket::SshIncomingPacket() : m_serverSeqNr(0) { }

quint32 SshIncomingPacket::cipherBlockSize() const
{
    return qMax(m_decrypter.cipherBlockSize(), 8U);
}

quint32 SshIncomingPacket::macLength() const
{
    return m_decrypter.macLength();
}

void SshIncomingPacket::recreateKeys(const SshKeyExchange &keyExchange)
{
    m_decrypter.recreateKeys(keyExchange);
}

void SshIncomingPacket::reset()
{
    clear();
    m_serverSeqNr = 0;
    m_decrypter.clearKeys();
}

void SshIncomingPacket::consumeData(QByteArray &newData)
{
#ifdef CREATOR_SSH_DEBUG
    qDebug("%s: current data size = %d, new data size = %d",
        Q_FUNC_INFO, m_data.size(), newData.size());
#endif

    if (isComplete() || newData.isEmpty())
        return;

    /*
     * Until we have reached the minimum packet size, we cannot decrypt the
     * length field.
     */
    const quint32 minSize = minPacketSize();
    if (currentDataSize() < minSize) {
        const int bytesToTake
            = qMin<quint32>(minSize - currentDataSize(), newData.size());
        moveFirstBytes(m_data, newData, bytesToTake);
#ifdef CREATOR_SSH_DEBUG
        qDebug("Took %d bytes from new data", bytesToTake);
#endif
        if (currentDataSize() < minSize)
            return;
    }

    if (4 + length() + macLength() < currentDataSize())
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR, "Server sent invalid packet.");

    const int bytesToTake
        = qMin<quint32>(length() + 4 + macLength() - currentDataSize(),
              newData.size());
    moveFirstBytes(m_data, newData, bytesToTake);
#ifdef CREATOR_SSH_DEBUG
    qDebug("Took %d bytes from new data", bytesToTake);
#endif
    if (isComplete()) {
#ifdef CREATOR_SSH_DEBUG
        qDebug("Message complete. Overall size: %u, payload size: %u",
            m_data.size(), m_length - paddingLength() - 1);
#endif
        decrypt();
        ++m_serverSeqNr;
    }
}

void SshIncomingPacket::decrypt()
{
    Q_ASSERT(isComplete());
    const quint32 netDataLength = length() + 4;
    m_decrypter.decrypt(m_data, cipherBlockSize(),
        netDataLength - cipherBlockSize());
    const QByteArray &mac = m_data.mid(netDataLength, macLength());
    if (mac != generateMac(m_decrypter, m_serverSeqNr)) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_MAC_ERROR,
                           "Message authentication failed.");
    }
}

void SshIncomingPacket::moveFirstBytes(QByteArray &target, QByteArray &source,
    int n)
{
    target.append(source.left(n));
    source.remove(0, n);
}

SshKeyExchangeInit SshIncomingPacket::extractKeyExchangeInitData() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_MSG_KEXINIT);

    SshKeyExchangeInit exchangeData;
    try {
        quint32 offset = TypeOffset + 1;
        std::memcpy(exchangeData.cookie, &m_data.constData()[offset],
                    sizeof exchangeData.cookie);
        offset += sizeof exchangeData.cookie;
        exchangeData.keyAlgorithms
            = SshPacketParser::asNameList(m_data, &offset);
        exchangeData.serverHostKeyAlgorithms
            = SshPacketParser::asNameList(m_data, &offset);
        exchangeData.encryptionAlgorithmsClientToServer
            = SshPacketParser::asNameList(m_data, &offset);
        exchangeData.encryptionAlgorithmsServerToClient
            = SshPacketParser::asNameList(m_data, &offset);
        exchangeData.macAlgorithmsClientToServer
            = SshPacketParser::asNameList(m_data, &offset);
        exchangeData.macAlgorithmsServerToClient
            = SshPacketParser::asNameList(m_data, &offset);
        exchangeData.compressionAlgorithmsClientToServer
            = SshPacketParser::asNameList(m_data, &offset);
        exchangeData.compressionAlgorithmsServerToClient
            = SshPacketParser::asNameList(m_data, &offset);
        exchangeData.languagesClientToServer
            = SshPacketParser::asNameList(m_data, &offset);
        exchangeData.languagesServerToClient
            = SshPacketParser::asNameList(m_data, &offset);
        exchangeData.firstKexPacketFollows
            = SshPacketParser::asBool(m_data, &offset);
    } catch (SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_KEY_EXCHANGE_FAILED,
            "Key exchange failed: Server sent invalid SSH_MSG_KEXINIT packet.");
    }
    return exchangeData;
}

SshKeyExchangeReply SshIncomingPacket::extractKeyExchangeReply(const QByteArray &pubKeyAlgo) const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_MSG_KEXDH_REPLY);

    try {
        SshKeyExchangeReply replyData;
        quint32 offset = TypeOffset + 1;
        const quint32 k_sLength
            = SshPacketParser::asUint32(m_data, &offset);
        if (offset + k_sLength > currentDataSize())
            throw SshPacketParseException();
        replyData.k_s = m_data.mid(offset - 4, k_sLength + 4);
        if (SshPacketParser::asString(m_data, &offset) != pubKeyAlgo)
            throw SshPacketParseException();

        // DSS: p and q, RSA: e and n
        replyData.parameters << SshPacketParser::asBigInt(m_data, &offset);
        replyData.parameters << SshPacketParser::asBigInt(m_data, &offset);

        // g and y
        if (pubKeyAlgo == SshCapabilities::PubKeyDss) {
            replyData.parameters << SshPacketParser::asBigInt(m_data, &offset);
            replyData.parameters << SshPacketParser::asBigInt(m_data, &offset);
        }

        replyData.f = SshPacketParser::asBigInt(m_data, &offset);
        offset += 4;
        if (SshPacketParser::asString(m_data, &offset) != pubKeyAlgo)
            throw SshPacketParseException();
        replyData.signatureBlob = SshPacketParser::asString(m_data, &offset);
        return replyData;
    } catch (SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_KEY_EXCHANGE_FAILED,
            "Key exchange failed: "
            "Server sent invalid SSH_MSG_KEXDH_REPLY packet.");
    }
}

SshDisconnect SshIncomingPacket::extractDisconnect() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_MSG_DISCONNECT);

    SshDisconnect msg;
    try {
        quint32 offset = TypeOffset + 1;
        msg.reasonCode = SshPacketParser::asUint32(m_data, &offset);
        msg.description = SshPacketParser::asUserString(m_data, &offset);
        msg.language = SshPacketParser::asString(m_data, &offset);
    } catch (SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid SSH_MSG_DISCONNECT.");
    }

    return msg;
}

SshUserAuthBanner SshIncomingPacket::extractUserAuthBanner() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_MSG_USERAUTH_BANNER);

    try {
        SshUserAuthBanner msg;
        quint32 offset = TypeOffset + 1;
        msg.message = SshPacketParser::asUserString(m_data, &offset);
        msg.language = SshPacketParser::asString(m_data, &offset);
        return msg;
    } catch (SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid SSH_MSG_USERAUTH_BANNER.");
    }
}

SshDebug SshIncomingPacket::extractDebug() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_MSG_DEBUG);

    try {
        SshDebug msg;
        quint32 offset = TypeOffset + 1;
        msg.display = SshPacketParser::asBool(m_data, &offset);
        msg.message = SshPacketParser::asUserString(m_data, &offset);
        msg.language = SshPacketParser::asString(m_data, &offset);
        return msg;
    } catch (SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid SSH_MSG_DEBUG.");
    }
}

SshUnimplemented SshIncomingPacket::extractUnimplemented() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_MSG_UNIMPLEMENTED);

    try {
        SshUnimplemented msg;
        quint32 offset = TypeOffset + 1;
        msg.invalidMsgSeqNr = SshPacketParser::asUint32(m_data, &offset);
        return msg;
    } catch (SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid SSH_MSG_UNIMPLEMENTED.");
    }
}

SshChannelOpenFailure SshIncomingPacket::extractChannelOpenFailure() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_MSG_CHANNEL_OPEN_FAILURE);

    SshChannelOpenFailure openFailure;
    try {
        quint32 offset = TypeOffset + 1;
        openFailure.localChannel = SshPacketParser::asUint32(m_data, &offset);
        openFailure.reasonCode = SshPacketParser::asUint32(m_data, &offset);
        openFailure.reasonString = QString::fromLocal8Bit(SshPacketParser::asString(m_data, &offset));
        openFailure.language = SshPacketParser::asString(m_data, &offset);
    } catch (SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Server sent invalid SSH_MSG_CHANNEL_OPEN_FAILURE packet.");
    }
    return openFailure;
}

SshChannelOpenConfirmation SshIncomingPacket::extractChannelOpenConfirmation() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_MSG_CHANNEL_OPEN_CONFIRMATION);

    SshChannelOpenConfirmation confirmation;
    try {
        quint32 offset = TypeOffset + 1;
        confirmation.localChannel = SshPacketParser::asUint32(m_data, &offset);
        confirmation.remoteChannel = SshPacketParser::asUint32(m_data, &offset);
        confirmation.remoteWindowSize = SshPacketParser::asUint32(m_data, &offset);
        confirmation.remoteMaxPacketSize = SshPacketParser::asUint32(m_data, &offset);
    } catch (SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Server sent invalid SSH_MSG_CHANNEL_OPEN_CONFIRMATION packet.");
    }
    return confirmation;
}

SshChannelWindowAdjust SshIncomingPacket::extractWindowAdjust() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_MSG_CHANNEL_WINDOW_ADJUST);

    SshChannelWindowAdjust adjust;
    try {
        quint32 offset = TypeOffset + 1;
        adjust.localChannel = SshPacketParser::asUint32(m_data, &offset);
        adjust.bytesToAdd = SshPacketParser::asUint32(m_data, &offset);
    } catch (SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid SSH_MSG_CHANNEL_WINDOW_ADJUST packet.");
    }
    return adjust;
}

SshChannelData SshIncomingPacket::extractChannelData() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_MSG_CHANNEL_DATA);

    SshChannelData data;
    try {
        quint32 offset = TypeOffset + 1;
        data.localChannel = SshPacketParser::asUint32(m_data, &offset);
        data.data = SshPacketParser::asString(m_data, &offset);
    } catch (SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid SSH_MSG_CHANNEL_DATA packet.");
    }
    return data;
}

SshChannelExtendedData SshIncomingPacket::extractChannelExtendedData() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_MSG_CHANNEL_EXTENDED_DATA);

    SshChannelExtendedData data;
    try {
        quint32 offset = TypeOffset + 1;
        data.localChannel = SshPacketParser::asUint32(m_data, &offset);
        data.type = SshPacketParser::asUint32(m_data, &offset);
        data.data = SshPacketParser::asString(m_data, &offset);
    } catch (SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid SSH_MSG_CHANNEL_EXTENDED_DATA packet.");
    }
    return data;
}

SshChannelExitStatus SshIncomingPacket::extractChannelExitStatus() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_MSG_CHANNEL_REQUEST);

    SshChannelExitStatus exitStatus;
    try {
        quint32 offset = TypeOffset + 1;
        exitStatus.localChannel = SshPacketParser::asUint32(m_data, &offset);
        const QByteArray &type = SshPacketParser::asString(m_data, &offset);
        Q_ASSERT(type == ExitStatusType);
        Q_UNUSED(type);
        if (SshPacketParser::asBool(m_data, &offset))
            throw SshPacketParseException();
        exitStatus.exitStatus = SshPacketParser::asUint32(m_data, &offset);
    } catch (SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid exit-status packet.");
    }
    return exitStatus;
}

SshChannelExitSignal SshIncomingPacket::extractChannelExitSignal() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_MSG_CHANNEL_REQUEST);

    SshChannelExitSignal exitSignal;
    try {
        quint32 offset = TypeOffset + 1;
        exitSignal.localChannel = SshPacketParser::asUint32(m_data, &offset);
        const QByteArray &type = SshPacketParser::asString(m_data, &offset);
        Q_ASSERT(type == ExitSignalType);
        Q_UNUSED(type);
        if (SshPacketParser::asBool(m_data, &offset))
            throw SshPacketParseException();
        exitSignal.signal = SshPacketParser::asString(m_data, &offset);
        exitSignal.coreDumped = SshPacketParser::asBool(m_data, &offset);
        exitSignal.error = SshPacketParser::asUserString(m_data, &offset);
        exitSignal.language = SshPacketParser::asString(m_data, &offset);
    } catch (SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid exit-signal packet.");
    }
    return exitSignal;
}

quint32 SshIncomingPacket::extractRecipientChannel() const
{
    Q_ASSERT(isComplete());

    try {
        quint32 offset = TypeOffset + 1;
        return SshPacketParser::asUint32(m_data, &offset);
    } catch (SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Server sent invalid packet.");
    }
}

QByteArray SshIncomingPacket::extractChannelRequestType() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_MSG_CHANNEL_REQUEST);

    try {
        quint32 offset = TypeOffset + 1;
        SshPacketParser::asUint32(m_data, &offset);
        return SshPacketParser::asString(m_data, &offset);
    } catch (SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid SSH_MSG_CHANNEL_REQUEST packet.");
    }
}

void SshIncomingPacket::calculateLength() const
{
    Q_ASSERT(currentDataSize() >= minPacketSize());
#ifdef CREATOR_SSH_DEBUG
    qDebug("Length field before decryption: %d-%d-%d-%d", m_data.at(0) & 0xff,
        m_data.at(1) & 0xff, m_data.at(2) & 0xff, m_data.at(3) & 0xff);
#endif
    m_decrypter.decrypt(m_data, 0, cipherBlockSize());
#ifdef CREATOR_SSH_DEBUG
    qDebug("Length field after decryption: %d-%d-%d-%d", m_data.at(0) & 0xff, m_data.at(1) & 0xff, m_data.at(2) & 0xff, m_data.at(3) & 0xff);
    qDebug("message type = %d", m_data.at(TypeOffset));
#endif
    m_length = SshPacketParser::asUint32(m_data, static_cast<quint32>(0));
#ifdef CREATOR_SSH_DEBUG
    qDebug("decrypted length is %u", m_length);
#endif
}

} // namespace Internal
} // namespace QSsh
