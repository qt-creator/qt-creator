/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
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

#include <QtCore/QDateTime>

namespace Utils {

using namespace Botan;
using namespace Internal;

SshKeyGenerator::SshKeyGenerator()
    : m_type(Rsa)
    , m_format(OpenSsl)
{
}

bool SshKeyGenerator::generateKeys(KeyType type, PrivateKeyFormat format,
    int keySize)
{
    m_type = type;
    m_format = format;

    try {
        AutoSeeded_RNG rng;
        KeyPtr key;
        if (m_type == Rsa)
            key = KeyPtr(new RSA_PrivateKey(rng, keySize));
        else
            key = KeyPtr(new DSA_PrivateKey(rng, DL_Group(rng, DL_Group::DSA_Kosherizer,
                keySize)));
        return m_format == Pkcs8
            ? generatePkcs8Keys(key) : generateOpenSslKeys(key);
    } catch (Botan::Exception &e) {
        m_error = tr("Error generating key: %1").arg(e.what());
        return false;
    }
}

bool SshKeyGenerator::generatePkcs8Keys(const KeyPtr &key)
{
    generatePkcs8Key(key, false);
    generatePkcs8Key(key, true);
    return true;
}

void SshKeyGenerator::generatePkcs8Key(const KeyPtr &key, bool privateKey)
{
    Pipe pipe;
    pipe.start_msg();
    QByteArray *keyData;
    if (privateKey) {
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

bool SshKeyGenerator::generateOpenSslKeys(const KeyPtr &key)
{
    QList<BigInt> publicParams;
    QList<BigInt> allParams;
    QByteArray keyId;
    if (m_type == Rsa) {
        const QSharedPointer<RSA_PrivateKey> rsaKey
            = key.dynamicCast<RSA_PrivateKey>();
        publicParams << rsaKey->get_e() << rsaKey->get_n();
        allParams << rsaKey->get_n() << rsaKey->get_e() << rsaKey->get_d()
            << rsaKey->get_p() << rsaKey->get_q();
        keyId = SshCapabilities::PubKeyRsa;
    } else {
        const QSharedPointer<DSA_PrivateKey> dsaKey
            = key.dynamicCast<DSA_PrivateKey>();
        publicParams << dsaKey->group_p() << dsaKey->group_q()
            << dsaKey->group_g() << dsaKey->get_y();
        allParams << publicParams << dsaKey->get_x();
        keyId = SshCapabilities::PubKeyDss;
    }

    QByteArray publicKeyBlob = AbstractSshPacket::encodeString(keyId);
    foreach (const BigInt &b, publicParams)
        publicKeyBlob += AbstractSshPacket::encodeMpInt(b);
    publicKeyBlob = publicKeyBlob.toBase64();
    const QByteArray id = "QtCreator/"
        + QDateTime::currentDateTime().toString(Qt::ISODate).toUtf8();
    m_publicKey = keyId + ' ' + publicKeyBlob + ' ' + id;

    DER_Encoder encoder;
    encoder.start_cons(SEQUENCE).encode (0U);
    foreach (const BigInt &b, allParams)
        encoder.encode(b);
    encoder.end_cons();
    const char * const label
        = m_type == Rsa ? "RSA PRIVATE KEY" : "DSA PRIVATE KEY";
    m_privateKey
        = QByteArray(PEM_Code::encode (encoder.get_contents(), label).c_str());
    return true;
}

} // namespace Utils
