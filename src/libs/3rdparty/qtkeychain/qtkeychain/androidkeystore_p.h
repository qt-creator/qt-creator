/******************************************************************************
 *   Copyright (C) 2016 Mathias Hasselmann <mathias.hasselmann@kdab.com>      *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution        *
 * details, check the accompanying file 'COPYING'.                            *
 *****************************************************************************/

#ifndef QTKEYCHAIN_ANDROIDKEYSTORE_P_H
#define QTKEYCHAIN_ANDROIDKEYSTORE_P_H

#include <QtGlobal>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#  include <QAndroidJniObject>
#else
#  include <QJniObject>
#  include <QJniEnvironment>

typedef QJniObject QAndroidJniObject;
typedef QJniEnvironment QAndroidJniEnvironment;

#endif

namespace QKeychain {

namespace javax {
namespace security {
namespace cert {
class Certificate;
}
} // namespace security
} // namespace javax

namespace java {
namespace lang {

class Object : protected QAndroidJniObject
{
public:
    inline Object(jobject object) : QAndroidJniObject(object) { }
    inline Object(const QAndroidJniObject &object) : QAndroidJniObject(object) { }
    inline operator bool() const { return isValid(); }

    using QAndroidJniObject::object;
    using QAndroidJniObject::toString;

protected:
    static bool handleExceptions();

    template <typename T>
    static T handleExceptions(const T &result, const T &resultOnError = T());
};

template <typename T>
inline T Object::handleExceptions(const T &result, const T &resultOnError)
{
    if (!handleExceptions())
        return resultOnError;

    return result;
}

} // namespace lang

namespace io {

class InputStream : public java::lang::Object
{
public:
    using Object::Object;

    int read() const;
    bool readAll(QByteArray &out, QString *errorString = nullptr) const;
};

class ByteArrayInputStream : public InputStream
{
public:
    using InputStream::InputStream;

    explicit ByteArrayInputStream(const QByteArray &bytes);
};

class FilterInputStream : public InputStream
{
public:
    using InputStream::InputStream;
};

class OutputStream : public java::lang::Object
{
public:
    using Object::Object;

    bool write(const QByteArray &bytes) const;
    bool flush() const;
    bool close() const;
};

class ByteArrayOutputStream : public OutputStream
{
public:
    using OutputStream::OutputStream;

    ByteArrayOutputStream();

    QByteArray toByteArray() const;
};

class FilterOutputStream : public OutputStream
{
public:
    using OutputStream::OutputStream;
};

} // namespace io

namespace security {
namespace spec {

class AlgorithmParameterSpec : public java::lang::Object
{
public:
    using Object::Object;
};

} // namespace spec

class Key : public java::lang::Object
{
public:
    using Object::Object;
};

class PrivateKey : public Key
{
public:
    using Key::Key;

    PrivateKey(const Key &init) : Key(init) { }
};

class PublicKey : public Key
{
public:
    using Key::Key;

    PublicKey(const Key &init) : Key(init) { }
};

class SecureRandom : public java::lang::Object
{
public:
    SecureRandom();
    bool nextBytes(QByteArray &bytes) const;
};

class KeyPair : public java::lang::Object
{
public:
    using Object::Object;
};

class KeyPairGenerator : public java::lang::Object
{
public:
    using Object::Object;

    static KeyPairGenerator getInstance(const QString &algorithm, const QString &provider);
    KeyPair generateKeyPair() const;
    bool initialize(const spec::AlgorithmParameterSpec &spec) const;
};

class KeyStore : public java::lang::Object
{
public:
    class Entry : public java::lang::Object
    {
    public:
        using Object::Object;
    };

    class PrivateKeyEntry : public Entry
    {
    public:
        using Entry::Entry;

        inline PrivateKeyEntry(const Entry &init) : Entry(init) { }

        javax::security::cert::Certificate getCertificate() const;
        java::security::PrivateKey getPrivateKey() const;
    };

    class LoadStoreParameter : public java::lang::Object
    {
    public:
        using Object::Object;
    };

    class ProtectionParameter : public java::lang::Object
    {
    public:
        using Object::Object;
    };

    using Object::Object;

    bool containsAlias(const QString &alias) const;
    bool deleteEntry(const QString &alias) const;
    static KeyStore getInstance(const QString &type);
    Entry getEntry(const QString &alias, const ProtectionParameter &param = nullptr) const;
    bool load(const LoadStoreParameter &param = nullptr) const;
};

namespace interfaces {

class RSAPrivateKey : public PrivateKey
{
public:
    using PrivateKey::PrivateKey;

    RSAPrivateKey(const PrivateKey &init) : PrivateKey(init) { }
};

class RSAPublicKey : public PublicKey
{
public:
    using PublicKey::PublicKey;

    RSAPublicKey(const PublicKey &init) : PublicKey(init) { }
};

} // namespace interfaces

} // namespace security
} // namespace java

namespace android {
namespace security {
namespace keystore {

namespace KeyProperties {
static const int PURPOSE_ENCRYPT = 1;
static const int PURPOSE_DECRYPT = 2;
} // namespace KeyProperties

class KeyGenParameterSpec : public java::security::spec::AlgorithmParameterSpec
{
public:
    class Builder : public java::lang::Object
    {
    public:
        using Object::Object;

        explicit Builder(const QString &keystoreAlias, int purposes);

        Builder setEncryptionPadding(const QString &padding) const;
        Builder setKeySize(int keySize) const;
        KeyGenParameterSpec build() const;
    };

    using AlgorithmParameterSpec::AlgorithmParameterSpec;
};

} // namespace keystore
} // namespace security
} // namespace android

namespace javax {
namespace crypto {

class SecretKeySpec : public java::security::Key
{
public:
    using Key::Key;

    explicit SecretKeySpec(const QByteArray &key, const QString &algorithm);
};

class GCMParameterSpec : public java::security::spec::AlgorithmParameterSpec
{
public:
    using AlgorithmParameterSpec::AlgorithmParameterSpec;

    explicit GCMParameterSpec(int tLen, const QByteArray &iv);
};

class Cipher : public java::lang::Object
{
public:
    static const int DECRYPT_MODE;
    static const int ENCRYPT_MODE;

    using Object::Object;

    static Cipher getInstance(const QString &transformation);
    bool init(int opMode, const java::security::Key &key) const;
    bool init(int opMode, const java::security::Key &key,
              const java::security::spec::AlgorithmParameterSpec &params) const;
    bool doFinal(const QByteArray &input, QByteArray &output,
                 QString *errorString = nullptr) const;
};

class CipherInputStream : public java::io::FilterInputStream
{
public:
    using FilterInputStream::FilterInputStream;

    explicit CipherInputStream(const InputStream &stream, const Cipher &cipher);
};

class CipherOutputStream : public java::io::FilterOutputStream
{
public:
    using FilterOutputStream::FilterOutputStream;

    explicit CipherOutputStream(const OutputStream &stream, const Cipher &cipher);
};

} // namespace crypto

namespace security {
namespace cert {

class Certificate : public java::lang::Object
{
public:
    using Object::Object;

    java::security::PublicKey getPublicKey() const;
};

} // namespace cert

} // namespace security
} // namespace javax

} // namespace QKeychain

#endif // QTKEYCHAIN_ANDROIDKEYSTORE_P_H
