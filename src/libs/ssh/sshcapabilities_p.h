/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CAPABILITIES_P_H
#define CAPABILITIES_P_H

#include <QByteArray>
#include <QList>

namespace QSsh {
namespace Internal {

class SshCapabilities
{
public:
    static const QByteArray DiffieHellmanGroup1Sha1;
    static const QByteArray DiffieHellmanGroup14Sha1;
    static const QList<QByteArray> KeyExchangeMethods;

    static const QByteArray PubKeyDss;
    static const QByteArray PubKeyRsa;
    static const QList<QByteArray> PublicKeyAlgorithms;

    static const QByteArray CryptAlgo3DesCbc;
    static const QByteArray CryptAlgo3DesCtr;
    static const QByteArray CryptAlgoAes128Cbc;
    static const QByteArray CryptAlgoAes128Ctr;
    static const QByteArray CryptAlgoAes192Ctr;
    static const QByteArray CryptAlgoAes256Ctr;
    static const QList<QByteArray> EncryptionAlgorithms;

    static const QByteArray HMacSha1;
    static const QByteArray HMacSha196;
    static const QList<QByteArray> MacAlgorithms;

    static const QList<QByteArray> CompressionAlgorithms;

    static const QByteArray SshConnectionService;

    static QByteArray findBestMatch(const QList<QByteArray> &myCapabilities,
        const QList<QByteArray> &serverCapabilities);
};

} // namespace Internal
} // namespace QSsh

#endif // CAPABILITIES_P_H
