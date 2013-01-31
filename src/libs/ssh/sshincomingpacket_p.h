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

#ifndef SSHINCOMINGPACKET_P_H
#define SSHINCOMINGPACKET_P_H

#include "sshpacket_p.h"

#include "sshcryptofacility_p.h"
#include "sshpacketparser_p.h"

#include <QList>
#include <QString>

namespace QSsh {
namespace Internal {

class SshKeyExchange;

struct SshKeyExchangeInit
{
    char cookie[16];
    SshNameList keyAlgorithms;
    SshNameList serverHostKeyAlgorithms;
    SshNameList encryptionAlgorithmsClientToServer;
    SshNameList encryptionAlgorithmsServerToClient;
    SshNameList macAlgorithmsClientToServer;
    SshNameList macAlgorithmsServerToClient;
    SshNameList compressionAlgorithmsClientToServer;
    SshNameList compressionAlgorithmsServerToClient;
    SshNameList languagesClientToServer;
    SshNameList languagesServerToClient;
    bool firstKexPacketFollows;
};

struct SshKeyExchangeReply
{
    QByteArray k_s;
    QList<Botan::BigInt> parameters; // DSS: p, q, g, y. RSA: e, n.
    Botan::BigInt f;
    QByteArray signatureBlob;
};

struct SshDisconnect
{
    quint32 reasonCode;
    QString description;
    QByteArray language;
};

struct SshUserAuthBanner
{
    QString message;
    QByteArray language;
};

struct SshDebug
{
    bool display;
    QString message;
    QByteArray language;
};

struct SshUnimplemented
{
    quint32 invalidMsgSeqNr;
};

struct SshChannelOpenFailure
{
    quint32 localChannel;
    quint32 reasonCode;
    QString reasonString;
    QByteArray language;
};

struct SshChannelOpenConfirmation
{
    quint32 localChannel;
    quint32 remoteChannel;
    quint32 remoteWindowSize;
    quint32 remoteMaxPacketSize;
};

struct SshChannelWindowAdjust
{
    quint32 localChannel;
    quint32 bytesToAdd;
};

struct SshChannelData
{
    quint32 localChannel;
    QByteArray data;
};

struct SshChannelExtendedData
{
    quint32 localChannel;
    quint32 type;
    QByteArray data;
};

struct SshChannelExitStatus
{
    quint32 localChannel;
    quint32 exitStatus;
};

struct SshChannelExitSignal
{
    quint32 localChannel;
    QByteArray signal;
    bool coreDumped;
    QString error;
    QByteArray language;
};


class SshIncomingPacket : public AbstractSshPacket
{
public:
    SshIncomingPacket();

    void consumeData(QByteArray &data);
    void recreateKeys(const SshKeyExchange &keyExchange);
    void reset();

    SshKeyExchangeInit extractKeyExchangeInitData() const;
    SshKeyExchangeReply extractKeyExchangeReply(const QByteArray &pubKeyAlgo) const;
    SshDisconnect extractDisconnect() const;
    SshUserAuthBanner extractUserAuthBanner() const;
    SshDebug extractDebug() const;
    SshUnimplemented extractUnimplemented() const;

    SshChannelOpenFailure extractChannelOpenFailure() const;
    SshChannelOpenConfirmation extractChannelOpenConfirmation() const;
    SshChannelWindowAdjust extractWindowAdjust() const;
    SshChannelData extractChannelData() const;
    SshChannelExtendedData extractChannelExtendedData() const;
    SshChannelExitStatus extractChannelExitStatus() const;
    SshChannelExitSignal extractChannelExitSignal() const;
    quint32 extractRecipientChannel() const;
    QByteArray extractChannelRequestType() const;

    quint32 serverSeqNr() const { return m_serverSeqNr; }

    static const QByteArray ExitStatusType;
    static const QByteArray ExitSignalType;

private:
    virtual quint32 cipherBlockSize() const;
    virtual quint32 macLength() const;
    virtual void calculateLength() const;

    void decrypt();
    void moveFirstBytes(QByteArray &target, QByteArray &source, int n);

    quint32 m_serverSeqNr;
    SshDecryptionFacility m_decrypter;
};

} // namespace Internal
} // namespace QSsh

#endif // SSHINCOMINGPACKET_P_H
