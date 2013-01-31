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

#ifndef BYTEARRAYCONVERSIONS_P_H
#define BYTEARRAYCONVERSIONS_P_H

#include "sshcapabilities_p.h"

#include <botan/botan.h>

namespace QSsh {
namespace Internal {

inline const Botan::byte *convertByteArray(const QByteArray &a)
{
    return reinterpret_cast<const Botan::byte *>(a.constData());
}

inline Botan::byte *convertByteArray(QByteArray &a)
{
    return reinterpret_cast<Botan::byte *>(a.data());
}

inline QByteArray convertByteArray(const Botan::SecureVector<Botan::byte> &v)
{
    return QByteArray(reinterpret_cast<const char *>(v.begin()), v.size());
}

inline const char *botanKeyExchangeAlgoName(const QByteArray &rfcAlgoName)
{
    Q_ASSERT(rfcAlgoName == SshCapabilities::DiffieHellmanGroup1Sha1
        || rfcAlgoName == SshCapabilities::DiffieHellmanGroup14Sha1);
    return rfcAlgoName == SshCapabilities::DiffieHellmanGroup1Sha1
        ? "modp/ietf/1024" : "modp/ietf/2048";
}

inline const char *botanCryptAlgoName(const QByteArray &rfcAlgoName)
{
    Q_ASSERT(rfcAlgoName == SshCapabilities::CryptAlgo3Des
        || rfcAlgoName == SshCapabilities::CryptAlgoAes128);
    return rfcAlgoName == SshCapabilities::CryptAlgo3Des
        ? "TripleDES" : "AES-128";
}

inline const char *botanEmsaAlgoName(const QByteArray &rfcAlgoName)
{
    Q_ASSERT(rfcAlgoName == SshCapabilities::PubKeyDss
        || rfcAlgoName == SshCapabilities::PubKeyRsa);
    return rfcAlgoName == SshCapabilities::PubKeyDss
        ? "EMSA1(SHA-1)" : "EMSA3(SHA-1)";
}

inline const char *botanSha1Name() { return "SHA-1"; }

inline const char *botanHMacAlgoName(const QByteArray &rfcAlgoName)
{
    Q_ASSERT(rfcAlgoName == SshCapabilities::HMacSha1);
    Q_UNUSED(rfcAlgoName);
    return botanSha1Name();
}

inline quint32 botanHMacKeyLen(const QByteArray &rfcAlgoName)
{
    Q_ASSERT(rfcAlgoName == SshCapabilities::HMacSha1);
    Q_UNUSED(rfcAlgoName);
    return 20;
}

} // namespace Internal
} // namespace QSsh

#endif // BYTEARRAYCONVERSIONS_P_H
