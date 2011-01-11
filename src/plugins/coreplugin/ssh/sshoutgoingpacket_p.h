/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SSHOUTGOINGPACKET_P_H
#define SSHOUTGOINGPACKET_P_H

#include "sshpacket_p.h"

namespace Core {
namespace Internal {

class SshEncryptionFacility;

class SshOutgoingPacket : public AbstractSshPacket
{
public:
    SshOutgoingPacket(const SshEncryptionFacility &encrypter,
        const quint32 &seqNr);

    void generateKeyExchangeInitPacket();
    void generateKeyDhInitPacket(const Botan::BigInt &e);
    void generateNewKeysPacket();
    void generateDisconnectPacket(SshErrorCode reason,
        const QByteArray &reasonString);
    void generateMsgUnimplementedPacket(quint32 serverSeqNr);
    void generateUserAuthServiceRequestPacket();
    void generateUserAuthByPwdRequestPacket(const QByteArray &user,
        const QByteArray &service, const QByteArray &pwd);
    void generateUserAuthByKeyRequestPacket(const QByteArray &user,
        const QByteArray &service);
    void generateRequestFailurePacket();
    void generateSessionPacket(quint32 channelId, quint32 windowSize,
        quint32 maxPacketSize);
    void generateEnvPacket(quint32 remoteChannel, const QByteArray &var,
        const QByteArray &value);
    void generateExecPacket(quint32 remoteChannel, const QByteArray &command);
    void generateSftpPacket(quint32 remoteChannel);
    void generateWindowAdjustPacket(quint32 remoteChannel, quint32 bytesToAdd);
    void generateChannelDataPacket(quint32 remoteChannel,
        const QByteArray &data);
    void generateChannelSignalPacket(quint32 remoteChannel,
        const QByteArray &signalName);
    void generateChannelEofPacket(quint32 remoteChannel);
    void generateChannelClosePacket(quint32 remoteChannel);

private:
    virtual quint32 cipherBlockSize() const;
    virtual quint32 macLength() const;

    static QByteArray encodeNameList(const QList<QByteArray> &list);

    void generateServiceRequest(const QByteArray &service);

    SshOutgoingPacket &init(SshPacketType type);
    SshOutgoingPacket &setPadding();
    SshOutgoingPacket &encrypt();
    void finalize();

    SshOutgoingPacket &appendInt(quint32 val);
    SshOutgoingPacket &appendString(const QByteArray &string);
    SshOutgoingPacket &appendMpInt(const Botan::BigInt &number);
    SshOutgoingPacket &appendBool(bool b);
    int sizeDivisor() const;

    const SshEncryptionFacility &m_encrypter;
    const quint32 &m_seqNr;
};

} // namespace Internal
} // namespace Core

#endif // SSHOUTGOINGPACKET_P_H
