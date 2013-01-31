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

#include "sshcapabilities_p.h"

#include "sshexception_p.h"

#include <QCoreApplication>
#include <QString>

namespace QSsh {
namespace Internal {

namespace {
    QByteArray listAsByteArray(const QList<QByteArray> &list)
    {
        QByteArray array;
        foreach (const QByteArray &elem, list)
            array += elem + ',';
        if (!array.isEmpty())
            array.remove(array.count() - 1, 1);
        return array;
    }
} // anonymous namspace

const QByteArray SshCapabilities::DiffieHellmanGroup1Sha1("diffie-hellman-group1-sha1");
const QByteArray SshCapabilities::DiffieHellmanGroup14Sha1("diffie-hellman-group14-sha1");
const QList<QByteArray> SshCapabilities::KeyExchangeMethods
    = QList<QByteArray>() << SshCapabilities::DiffieHellmanGroup1Sha1
          << SshCapabilities::DiffieHellmanGroup14Sha1;

const QByteArray SshCapabilities::PubKeyDss("ssh-dss");
const QByteArray SshCapabilities::PubKeyRsa("ssh-rsa");
const QList<QByteArray> SshCapabilities::PublicKeyAlgorithms
    = QList<QByteArray>() << SshCapabilities::PubKeyRsa
          << SshCapabilities::PubKeyDss;

const QByteArray SshCapabilities::CryptAlgo3Des("3des-cbc");
const QByteArray SshCapabilities::CryptAlgoAes128("aes128-cbc");
const QList<QByteArray> SshCapabilities::EncryptionAlgorithms
    = QList<QByteArray>() << SshCapabilities::CryptAlgoAes128
          << SshCapabilities::CryptAlgo3Des;

const QByteArray SshCapabilities::HMacSha1("hmac-sha1");
const QByteArray SshCapabilities::HMacSha196("hmac-sha1-96");
const QList<QByteArray> SshCapabilities::MacAlgorithms
    = QList<QByteArray>() /* << SshCapabilities::HMacSha196 */
        << SshCapabilities::HMacSha1;

const QList<QByteArray> SshCapabilities::CompressionAlgorithms
    = QList<QByteArray>() << "none";

const QByteArray SshCapabilities::SshConnectionService("ssh-connection");

const QByteArray SshCapabilities::PublicKeyAuthMethod("publickey");
const QByteArray SshCapabilities::PasswordAuthMethod("password");


QByteArray SshCapabilities::findBestMatch(const QList<QByteArray> &myCapabilities,
    const QList<QByteArray> &serverCapabilities)
{
    foreach (const QByteArray &myCapability, myCapabilities) {
        if (serverCapabilities.contains(myCapability))
            return myCapability;
    }

    throw SshServerException(SSH_DISCONNECT_KEY_EXCHANGE_FAILED,
        "Server and client capabilities do not match.",
        QCoreApplication::translate("SshConnection",
            "Server and client capabilities don't match. "
            "Client list was: %1.\nServer list was %2.")
            .arg(QString::fromLocal8Bit(listAsByteArray(myCapabilities).data()))
            .arg(QString::fromLocal8Bit(listAsByteArray(serverCapabilities).data())));
}

} // namespace Internal
} // namespace QSsh
