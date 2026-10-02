/******************************************************************************
 *   Copyright (C) 2016 Mathias Hasselmann <mathias.hasselmann@kdab.com>      *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution        *
 * details, check the accompanying file 'COPYING'.                            *
 *****************************************************************************/

#include "keychain_p.h"

#include "androidkeystore_p.h"
#include "plaintextstore_p.h"

using namespace QKeychain;

using android::security::keystore::KeyGenParameterSpec;

using java::io::ByteArrayInputStream;
using java::security::SecureRandom;
using java::security::KeyPair;
using java::security::KeyPairGenerator;
using java::security::KeyStore;
using java::security::interfaces::RSAPublicKey;

using javax::crypto::Cipher;
using javax::crypto::CipherInputStream;
using javax::crypto::GCMParameterSpec;
using javax::crypto::SecretKeySpec;

namespace {

inline QString makeAlias(const QString &service, const QString &key)
{
    return service + QLatin1Char('/') + key;
}

// Magic prefix identifying the hybrid RSA+AES-GCM format (v2).
// Legacy entries have no prefix and are raw RSA ciphertext.
const QByteArray kHybridMagic("QKCA", 4);

} // namespace

void ReadPasswordJobPrivate::scheduledStart()
{
    PlainTextStore plainTextStore(q->service(), q->settings());

    if (!plainTextStore.contains(q->key())) {
        q->emitFinishedWithError(Error::EntryNotFound, tr("Entry not found"));
        return;
    }

    const QByteArray &encryptedData = plainTextStore.readData(q->key());
    const auto keyStore = KeyStore::getInstance(QStringLiteral("AndroidKeyStore"));

    if (!keyStore || !keyStore.load()) {
        q->emitFinishedWithError(Error::AccessDenied, tr("Could not open keystore"));
        return;
    }

    const auto &alias = makeAlias(q->service(), q->key());
    const KeyStore::PrivateKeyEntry entry = keyStore.getEntry(alias);

    if (!entry) {
        q->emitFinishedWithError(Error::AccessDenied,
                                 tr("Could not retrieve private key from keystore"));
        return;
    }

    QByteArray plainData;

    if (encryptedData.startsWith(kHybridMagic)) {
        // Hybrid format: kHybridMagic(4) + encKeyLen(4 BE) + RSA(AESkey) + IV(12) + AES-GCM ciphertext
        const int minSize = kHybridMagic.size() + 4 + 1 + 12 + 16;
        if (encryptedData.size() < minSize) {
            q->emitFinishedWithError(Error::OtherError, tr("Encrypted data is too short"));
            return;
        }

        const int lenOffset = kHybridMagic.size();
        const quint32 encKeyLen =
                (static_cast<quint32>(static_cast<unsigned char>(encryptedData[lenOffset])) << 24)
                | (static_cast<quint32>(static_cast<unsigned char>(encryptedData[lenOffset + 1])) << 16)
                | (static_cast<quint32>(static_cast<unsigned char>(encryptedData[lenOffset + 2])) << 8)
                | (static_cast<quint32>(static_cast<unsigned char>(encryptedData[lenOffset + 3])));

        const int dataOffset = lenOffset + 4;
        if (encryptedData.size() < dataOffset + (int)encKeyLen + 12 + 16) {
            q->emitFinishedWithError(Error::OtherError, tr("Encrypted data is too short"));
            return;
        }

        const QByteArray encryptedKey = encryptedData.mid(dataOffset, encKeyLen);
        const QByteArray iv = encryptedData.mid(dataOffset + encKeyLen, 12);
        const QByteArray encryptedPayload = encryptedData.mid(dataOffset + encKeyLen + 12);

        // Decrypt the AES key with RSA
        const auto rsaCipher = Cipher::getInstance(QStringLiteral("RSA/ECB/PKCS1Padding"));
        if (!rsaCipher || !rsaCipher.init(Cipher::DECRYPT_MODE, entry.getPrivateKey())) {
            q->emitFinishedWithError(Error::OtherError, tr("Could not create RSA decryption cipher"));
            return;
        }

        QByteArray aesKeyBytes;
        QString decryptError;
        if (!rsaCipher.doFinal(encryptedKey, aesKeyBytes, &decryptError)) {
            q->emitFinishedWithError(Error::OtherError,
                                     tr("Could not decrypt AES key: %1").arg(decryptError));
            return;
        }

        // Decrypt the payload with AES-GCM
        const SecretKeySpec aesKey(aesKeyBytes, QStringLiteral("AES"));
        const GCMParameterSpec gcmSpec(128, iv);
        const auto aesCipher = Cipher::getInstance(QStringLiteral("AES/GCM/NoPadding"));
        if (!aesCipher || !aesCipher.init(Cipher::DECRYPT_MODE, aesKey, gcmSpec)) {
            q->emitFinishedWithError(Error::OtherError,
                                     tr("Could not create AES decryption cipher"));
            return;
        }

        if (!aesCipher.doFinal(encryptedPayload, plainData, &decryptError)) {
            q->emitFinishedWithError(Error::OtherError,
                                     tr("Could not decrypt data: %1").arg(decryptError));
            return;
        }
    } else {
        // Legacy format: raw RSA-encrypted blob (only works for data <= ~245 bytes)
        const auto cipher = Cipher::getInstance(QStringLiteral("RSA/ECB/PKCS1Padding"));
        if (!cipher || !cipher.init(Cipher::DECRYPT_MODE, entry.getPrivateKey())) {
            q->emitFinishedWithError(Error::OtherError, tr("Could not create decryption cipher"));
            return;
        }

        const CipherInputStream inputStream(ByteArrayInputStream(encryptedData), cipher);
        QString readError;
        if (!inputStream.readAll(plainData, &readError)) {
            q->emitFinishedWithError(Error::OtherError,
                                     tr("Could not decrypt data: %1").arg(readError));
            return;
        }
    }

    mode = plainTextStore.readMode(q->key());
    data = plainData;
    q->emitFinished();
}

void WritePasswordJobPrivate::scheduledStart()
{
    const KeyStore keyStore = KeyStore::getInstance(QStringLiteral("AndroidKeyStore"));

    if (!keyStore || !keyStore.load()) {
        q->emitFinishedWithError(Error::AccessDenied, tr("Could not open keystore"));
        return;
    }

    const auto &alias = makeAlias(q->service(), q->key());
    if (!keyStore.containsAlias(alias)) {
        const KeyGenParameterSpec spec =
                KeyGenParameterSpec::Builder(
                        alias,
                        android::security::keystore::KeyProperties::PURPOSE_ENCRYPT
                                | android::security::keystore::KeyProperties::PURPOSE_DECRYPT)
                        .setEncryptionPadding(QStringLiteral("PKCS1Padding"))
                        .setKeySize(2048)
                        .build();

        const auto generator = KeyPairGenerator::getInstance(QStringLiteral("RSA"),
                                                             QStringLiteral("AndroidKeyStore"));

        if (!generator) {
            q->emitFinishedWithError(Error::OtherError,
                                     tr("Could not create private key generator"));
            return;
        }

        generator.initialize(spec);

        if (!generator.generateKeyPair()) {
            q->emitFinishedWithError(Error::OtherError, tr("Could not generate new private key"));
            return;
        }
    }

    const KeyStore::PrivateKeyEntry entry = keyStore.getEntry(alias);

    if (!entry) {
        q->emitFinishedWithError(Error::AccessDenied,
                                 tr("Could not retrieve private key from keystore"));
        return;
    }

    const RSAPublicKey publicKey = entry.getCertificate().getPublicKey();

    // Generate a random AES-256 key
    QByteArray aesKeyBytes(32, '\0');
    SecureRandom secureRandom;
    if (!secureRandom || !secureRandom.nextBytes(aesKeyBytes)) {
        q->emitFinishedWithError(Error::OtherError, tr("Could not generate AES key"));
        return;
    }

    // Generate a random 12-byte IV for AES-GCM
    QByteArray iv(12, '\0');
    if (!secureRandom.nextBytes(iv)) {
        q->emitFinishedWithError(Error::OtherError, tr("Could not generate IV"));
        return;
    }

    // Encrypt the payload with AES/GCM/NoPadding
    const SecretKeySpec aesKey(aesKeyBytes, QStringLiteral("AES"));
    const GCMParameterSpec gcmSpec(128, iv);
    const auto aesCipher = Cipher::getInstance(QStringLiteral("AES/GCM/NoPadding"));
    if (!aesCipher || !aesCipher.init(Cipher::ENCRYPT_MODE, aesKey, gcmSpec)) {
        q->emitFinishedWithError(Error::OtherError, tr("Could not create AES encryption cipher"));
        return;
    }

    QByteArray encryptedPayload;
    QString encryptError;
    if (!aesCipher.doFinal(data, encryptedPayload, &encryptError)) {
        q->emitFinishedWithError(Error::OtherError,
                                 tr("Could not encrypt data: %1").arg(encryptError));
        return;
    }

    // Encrypt the AES key with RSA (32 bytes always fits within RSA-2048 limit)
    const auto rsaCipher = Cipher::getInstance(QStringLiteral("RSA/ECB/PKCS1Padding"));
    if (!rsaCipher || !rsaCipher.init(Cipher::ENCRYPT_MODE, publicKey)) {
        q->emitFinishedWithError(Error::OtherError, tr("Could not create RSA encryption cipher"));
        return;
    }

    QByteArray encryptedKey;
    if (!rsaCipher.doFinal(aesKeyBytes, encryptedKey, &encryptError)) {
        q->emitFinishedWithError(Error::OtherError,
                                 tr("Could not encrypt AES key: %1").arg(encryptError));
        return;
    }

    // Assemble blob: kHybridMagic(4) + encKeyLen(4 BE) + encryptedKey + iv(12) + encryptedPayload
    const quint32 encKeyLen = static_cast<quint32>(encryptedKey.size());
    QByteArray blob;
    blob.reserve(kHybridMagic.size() + 4 + encryptedKey.size() + iv.size()
                 + encryptedPayload.size());
    blob += kHybridMagic;
    blob += static_cast<char>((encKeyLen >> 24) & 0xFF);
    blob += static_cast<char>((encKeyLen >> 16) & 0xFF);
    blob += static_cast<char>((encKeyLen >> 8) & 0xFF);
    blob += static_cast<char>(encKeyLen & 0xFF);
    blob += encryptedKey;
    blob += iv;
    blob += encryptedPayload;

    PlainTextStore plainTextStore(q->service(), q->settings());
    plainTextStore.write(q->key(), blob, mode);

    if (plainTextStore.error() != NoError)
        q->emitFinishedWithError(plainTextStore.error(), plainTextStore.errorString());
    else
        q->emitFinished();
}

void DeletePasswordJobPrivate::scheduledStart()
{
    const auto keyStore = KeyStore::getInstance(QStringLiteral("AndroidKeyStore"));

    if (!keyStore || !keyStore.load()) {
        q->emitFinishedWithError(Error::AccessDenied, tr("Could not open keystore"));
        return;
    }

    const auto &alias = makeAlias(q->service(), q->key());
    if (!keyStore.deleteEntry(alias)) {
        q->emitFinishedWithError(Error::OtherError,
                                 tr("Could not remove private key from keystore"));
        return;
    }

    PlainTextStore plainTextStore(q->service(), q->settings());
    plainTextStore.remove(q->key());

    if (plainTextStore.error() != NoError)
        q->emitFinishedWithError(plainTextStore.error(), plainTextStore.errorString());
    else
        q->emitFinished();
}

bool QKeychain::isAvailable()
{
    return true;
}
