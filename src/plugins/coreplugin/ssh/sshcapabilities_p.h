/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CAPABILITIES_P_H
#define CAPABILITIES_P_H

#include <QtCore/QByteArray>
#include <QtCore/QList>

namespace Core {
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

    static const QByteArray CryptAlgo3Des;
    static const QByteArray CryptAlgoAes128;
    static const QList<QByteArray> EncryptionAlgorithms;

    static const QByteArray HMacSha1;
    static const QByteArray HMacSha196;
    static const QList<QByteArray> MacAlgorithms;

    static const QList<QByteArray> CompressionAlgorithms;

    static const QByteArray SshConnectionService;

    static const QByteArray PublicKeyAuthMethod;
    static const QByteArray PasswordAuthMethod;

    static QByteArray findBestMatch(const QList<QByteArray> &myCapabilities,
        const QList<QByteArray> &serverCapabilities);
};

} // namespace Internal
} // namespace Core

#endif // CAPABILITIES_P_H
