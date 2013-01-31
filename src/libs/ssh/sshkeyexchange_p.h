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

#ifndef SSHKEYEXCHANGE_P_H
#define SSHKEYEXCHANGE_P_H

#include <QByteArray>
#include <QScopedPointer>

namespace Botan {
class DH_PrivateKey;
class HashFunction;
}

namespace QSsh {
namespace Internal {

class SshSendFacility;
class SshIncomingPacket;

class SshKeyExchange
{
public:
    SshKeyExchange(SshSendFacility &sendFacility);
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
    QByteArray m_serverId;
    QByteArray m_clientKexInitPayload;
    QByteArray m_serverKexInitPayload;
    QScopedPointer<Botan::DH_PrivateKey> m_dhKey;
    QByteArray m_k;
    QByteArray m_h;
    QByteArray m_serverHostKeyAlgo;
    QByteArray m_encryptionAlgo;
    QByteArray m_decryptionAlgo;
    QByteArray m_c2sHMacAlgo;
    QByteArray m_s2cHMacAlgo;
    QScopedPointer<Botan::HashFunction> m_hash;
    SshSendFacility &m_sendFacility;
};

} // namespace Internal
} // namespace QSsh

#endif // SSHKEYEXCHANGE_P_H
