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

#ifndef SSHKEYEXCHANGE_P_H
#define SSHKEYEXCHANGE_P_H

#include "sshconnection.h"

#include <QByteArray>
#include <QScopedPointer>

namespace Botan {
class DH_PrivateKey;
class ECDH_PrivateKey;
class HashFunction;
}

namespace QSsh {
namespace Internal {

struct SshKeyExchangeInit;
class SshSendFacility;
class SshIncomingPacket;

class SshKeyExchange
{
public:
    SshKeyExchange(const SshConnectionParameters &connParams, SshSendFacility &sendFacility);
    ~SshKeyExchange();

    void sendKexInitPacket(const QByteArray &serverId);

    // Returns true <=> the server sends a guessed package.
    bool sendDhInitPacket(const SshIncomingPacket &serverKexInit);

    void sendNewKeysPacket(const SshIncomingPacket &dhReply,
        const QByteArray &clientId);

    QByteArray k() const { return m_k; }
    QByteArray h() const { return m_h; }
    Botan::HashFunction *hash() const { return m_hash.data(); }
    QByteArray encryptionAlgo() const { return m_encryptionAlgo; }
    QByteArray decryptionAlgo() const { return m_decryptionAlgo; }
    QByteArray hMacAlgoClientToServer() const { return m_c2sHMacAlgo; }
    QByteArray hMacAlgoServerToClient() const { return m_s2cHMacAlgo; }

private:
    QByteArray hashAlgoForKexAlgo() const;
    void determineHashingAlgorithm(const SshKeyExchangeInit &kexInit, bool serverToClient);
    void checkHostKey(const QByteArray &hostKey);
    Q_NORETURN void throwHostKeyException();

    QByteArray m_serverId;
    QByteArray m_clientKexInitPayload;
    QByteArray m_serverKexInitPayload;
    QScopedPointer<Botan::DH_PrivateKey> m_dhKey;
    QScopedPointer<Botan::ECDH_PrivateKey> m_ecdhKey;
    QByteArray m_kexAlgoName;
    QByteArray m_k;
    QByteArray m_h;
    QByteArray m_serverHostKeyAlgo;
    QByteArray m_encryptionAlgo;
    QByteArray m_decryptionAlgo;
    QByteArray m_c2sHMacAlgo;
    QByteArray m_s2cHMacAlgo;
    QScopedPointer<Botan::HashFunction> m_hash;
    const SshConnectionParameters m_connParams;
    SshSendFacility &m_sendFacility;
};

} // namespace Internal
} // namespace QSsh

#endif // SSHKEYEXCHANGE_P_H
