/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "sshkeygenerator.h"

#include "sshbotanconversions_p.h"
#include "sshcapabilities_p.h"
#include "sshpacket_p.h"

#include <botan/auto_rng.h>
#include <botan/bigint.h>
#include <botan/der_enc.h>
#include <botan/dsa.h>
#include <botan/pem.h>
#include <botan/pkcs8.h>
#include <botan/rsa.h>
#include <botan/x509_key.h>

#include <QDateTime>
#include <QInputDialog>

#include <string>

namespace QSsh {

using namespace Botan;
using namespace Internal;

SshKeyGenerator::SshKeyGenerator() : m_type(Rsa)
{
}

bool SshKeyGenerator::generateKeys(KeyType type, PrivateKeyFormat format, int keySize,
    EncryptionMode encryptionMode)
{
    m_type = type;
    m_encryptionMode = encryptionMode;

    try {
        AutoSeeded_RNG rng;
        KeyPtr key;
        if (m_type == Rsa)
            key = KeyPtr(new RSA_PrivateKey(rng, keySize));
        else
            key = KeyPtr(new DSA_PrivateKey(rng, DL_Group(rng, DL_Group::DSA_Kosherizer, keySize)));
        switch (format) {
        case Pkcs8:
            generatePkcs8KeyStrings(key, rng);
            break;
        case OpenSsl:
            generateOpenSslKeyStrings(key);
            break;
        case Mixed:
        default:
            generatePkcs8KeyString(key, true, rng);
            generateOpenSslPublicKeyString(key);
        }
        return true;
    } catch (Botan::Exception &e) {
        m_error = tr("Error generating key: %1").arg(QString::fromAscii(e.what()));
        return false;
    }
}

void SshKeyGenerator::generatePkcs8KeyStrings(const KeyPtr &key, Botan::RandomNumberGenerator &rng)
{
    generatePkcs8KeyString(key, false, rng);
    generatePkcs8KeyString(key, true, rng);
}

void SshKeyGenerator::generatePkcs8KeyString(const KeyPtr &key, bool privateKey,
    Botan::RandomNumberGenerator &rng)
{
    Pipe pipe;
    pipe.start_msg();
    QByteArray *keyData;
    if (privateKey) {
        QString password;
        if (m_encryptionMode == DoOfferEncryption)
            password = getPassword();
        if (!password.isEmpty())
            PKCS8::encrypt_key(*key, pipe, rng, password.toLocal8Bit().data());
        else
            PKCS8::encode(*key, pipe);
        keyData = &m_privateKey;
    } else {
        X509::encode(*key, pipe);
        keyData = &m_publicKey;
    }
    pipe.end_msg();
    keyData->resize(pipe.remaining(pipe.message_count() - 1));
    pipe.read(convertByteArray(*keyData), keyData->size(),
        pipe.message_count() - 1);
}

void SshKeyGenerator::generateOpenSslKeyStrings(const KeyPtr &key)
{
    generateOpenSslPublicKeyString(key);
    generateOpenSslPrivateKeyString(key);
}

void SshKeyGenerator::generateOpenSslPublicKeyString(const KeyPtr &key)
{
    QList<BigInt> params;
    QByteArray keyId;
    if (m_type == Rsa) {
        const QSharedPointer<RSA_PrivateKey> rsaKey = key.dynamicCast<RSA_PrivateKey>();
        params << rsaKey->get_e() << rsaKey->get_n();
        keyId = SshCapabilities::PubKeyRsa;
    } else {
        const QSharedPointer<DSA_PrivateKey> dsaKey = key.dynamicCast<DSA_PrivateKey>();
        params << dsaKey->group_p() << dsaKey->group_q() << dsaKey->group_g() << dsaKey->get_y();
        keyId = SshCapabilities::PubKeyDss;
    }

    QByteArray publicKeyBlob = AbstractSshPacket::encodeString(keyId);
    foreach (const BigInt &b, params)
        publicKeyBlob += AbstractSshPacket::encodeMpInt(b);
    publicKeyBlob = publicKeyBlob.toBase64();
    const QByteArray id = "QtCreator/"
        + QDateTime::currentDateTime().toString(Qt::ISODate).toUtf8();
    m_publicKey = keyId + ' ' + publicKeyBlob + ' ' + id;
}

void SshKeyGenerator::generateOpenSslPrivateKeyString(const KeyPtr &key)
{
    QList<BigInt> params;
    QByteArray keyId;
    const char *label;
    if (m_type == Rsa) {
        const QSharedPointer<RSA_PrivateKey> rsaKey
            = key.dynamicCast<RSA_PrivateKey>();
        params << rsaKey->get_n() << rsaKey->get_e() << rsaKey->get_d() << rsaKey->get_p()
            << rsaKey->get_q();
        keyId = SshCapabilities::PubKeyRsa;
        label = "RSA PRIVATE KEY";
    } else {
        const QSharedPointer<DSA_PrivateKey> dsaKey = key.dynamicCast<DSA_PrivateKey>();
        params << dsaKey->group_p() << dsaKey->group_q() << dsaKey->group_g() << dsaKey->get_y()
            << dsaKey->get_x();
        keyId = SshCapabilities::PubKeyDss;
        label = "DSA PRIVATE KEY";
    }

    DER_Encoder encoder;
    encoder.start_cons(SEQUENCE).encode(0U);
    foreach (const BigInt &b, params)
        encoder.encode(b);
    encoder.end_cons();
    m_privateKey = QByteArray(PEM_Code::encode (encoder.get_contents(), label).c_str());
}

QString SshKeyGenerator::getPassword() const
{
    QInputDialog d;
    d.setInputMode(QInputDialog::TextInput);
    d.setTextEchoMode(QLineEdit::Password);
    d.setWindowTitle(tr("Password for Private Key"));
    d.setLabelText(tr("It is recommended that you secure your private key\n"
        "with a password, which you can enter below."));
    d.setOkButtonText(tr("Encrypt Key File"));
    d.setCancelButtonText(tr("Do Not Encrypt Key File"));
    int result = QDialog::Accepted;
    QString password;
    while (result == QDialog::Accepted && password.isEmpty()) {
        result = d.exec();
        password = d.textValue();
    }
    return result == QDialog::Accepted ? password : QString();
}

} // namespace QSsh
