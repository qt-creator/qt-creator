#include "androidkeystore_p.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
#  include "private/qjni_p.h"
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#  include <QAndroidJniEnvironment>
#endif

using namespace QKeychain;

using namespace java::io;
using namespace java::lang;
using namespace java::security;
using namespace java::security::spec;

using namespace javax::crypto;
using namespace javax::security::cert;

using namespace android::security::keystore;

const int Cipher::DECRYPT_MODE =
        Cipher::getStaticField<jint>("javax/crypto/Cipher", "DECRYPT_MODE");
const int Cipher::ENCRYPT_MODE =
        Cipher::getStaticField<jint>("javax/crypto/Cipher", "ENCRYPT_MODE");

namespace {

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)

struct JNIObject
{
    JNIObject(QSharedPointer<QJNIObjectPrivate> d) : d(d) { }

    static JNIObject fromLocalRef(jobject o)
    {
        return JNIObject(
                QSharedPointer<QJNIObjectPrivate>::create(QJNIObjectPrivate::fromLocalRef(o)));
    }

    jobject object() const { return d->object(); }
    QSharedPointer<QJNIObjectPrivate> d;
};

#else

using JNIObject = QAndroidJniObject;

#endif

QByteArray fromArray(const jbyteArray array)
{
    QAndroidJniEnvironment env;
    jbyte *const bytes = env->GetByteArrayElements(array, nullptr);
    const QByteArray result(reinterpret_cast<const char *>(bytes), env->GetArrayLength(array));
    env->ReleaseByteArrayElements(array, bytes, JNI_ABORT);
    return result;
}

JNIObject toArray(const QByteArray &bytes)
{
    QAndroidJniEnvironment env;
    const int length = bytes.length();
    JNIObject array = JNIObject::fromLocalRef(env->NewByteArray(length));
    env->SetByteArrayRegion(static_cast<jbyteArray>(array.object()), 0, length,
                            reinterpret_cast<const jbyte *>(bytes.constData()));
    return array;
}

} // namespace

bool Object::handleExceptions()
{
    QAndroidJniEnvironment env;

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return false;
    }

    return true;
}

KeyPairGenerator KeyPairGenerator::getInstance(const QString &algorithm, const QString &provider)
{
    return handleExceptions(callStaticObjectMethod(
            "java/security/KeyPairGenerator", "getInstance",
            "(Ljava/lang/String;Ljava/lang/String;)Ljava/security/KeyPairGenerator;",
            fromString(algorithm).object(), fromString(provider).object()));
}

KeyPair KeyPairGenerator::generateKeyPair() const
{
    return handleExceptions(callObjectMethod("generateKeyPair", "()Ljava/security/KeyPair;"));
}

bool KeyPairGenerator::initialize(const AlgorithmParameterSpec &spec) const
{
    callMethod<void>("initialize", "(Ljava/security/spec/AlgorithmParameterSpec;)V", spec.object());
    return handleExceptions();
}

bool KeyStore::containsAlias(const QString &alias) const
{
    return handleExceptions(callMethod<jboolean>("containsAlias", "(Ljava/lang/String;)Z",
                                                 fromString(alias).object()));
}

bool KeyStore::deleteEntry(const QString &alias) const
{
    callMethod<void>("deleteEntry", "(Ljava/lang/String;)V", fromString(alias).object());
    return handleExceptions();
}

KeyStore KeyStore::getInstance(const QString &type)
{
    return handleExceptions(callStaticObjectMethod("java/security/KeyStore", "getInstance",
                                                   "(Ljava/lang/String;)Ljava/security/KeyStore;",
                                                   fromString(type).object()));
}

KeyStore::Entry KeyStore::getEntry(const QString &alias,
                                   const KeyStore::ProtectionParameter &param) const
{
    return handleExceptions(
            callObjectMethod("getEntry",
                             "(Ljava/lang/String;Ljava/security/"
                             "KeyStore$ProtectionParameter;)Ljava/security/KeyStore$Entry;",
                             fromString(alias).object(), param.object()));
}

bool KeyStore::load(const KeyStore::LoadStoreParameter &param) const
{
    callMethod<void>("load", "(Ljava/security/KeyStore$LoadStoreParameter;)V", param.object());
    return handleExceptions();
}

KeyGenParameterSpec::Builder::Builder(const QString &keystoreAlias, int purposes)
    : Object(QAndroidJniObject("android/security/keystore/KeyGenParameterSpec$Builder",
                               "(Ljava/lang/String;I)V",
                               fromString(keystoreAlias).object(),
                               static_cast<jint>(purposes)))
{
    handleExceptions();
}

KeyGenParameterSpec::Builder
KeyGenParameterSpec::Builder::setEncryptionPadding(const QString &padding) const
{
    QAndroidJniEnvironment env;
    const jclass stringClass = env->FindClass("java/lang/String");
    const jobjectArray arr = env->NewObjectArray(1, stringClass, nullptr);
    env->DeleteLocalRef(stringClass);
    const QAndroidJniObject str = fromString(padding);
    env->SetObjectArrayElement(arr, 0, str.object());
    const KeyGenParameterSpec::Builder result = handleExceptions(callObjectMethod(
            "setEncryptionPaddings",
            "([Ljava/lang/String;)Landroid/security/keystore/KeyGenParameterSpec$Builder;",
            arr));
    env->DeleteLocalRef(arr);
    return result;
}

KeyGenParameterSpec::Builder KeyGenParameterSpec::Builder::setKeySize(int keySize) const
{
    return handleExceptions(callObjectMethod(
            "setKeySize",
            "(I)Landroid/security/keystore/KeyGenParameterSpec$Builder;",
            static_cast<jint>(keySize)));
}

KeyGenParameterSpec KeyGenParameterSpec::Builder::build() const
{
    return handleExceptions(
            callObjectMethod("build", "()Landroid/security/keystore/KeyGenParameterSpec;"));
}

Certificate KeyStore::PrivateKeyEntry::getCertificate() const
{
    return handleExceptions(
            callObjectMethod("getCertificate", "()Ljava/security/cert/Certificate;"));
}

PrivateKey KeyStore::PrivateKeyEntry::getPrivateKey() const
{
    return handleExceptions(callObjectMethod("getPrivateKey", "()Ljava/security/PrivateKey;"));
}

PublicKey Certificate::getPublicKey() const
{
    return handleExceptions(callObjectMethod("getPublicKey", "()Ljava/security/PublicKey;"));
}

ByteArrayInputStream::ByteArrayInputStream(const QByteArray &bytes)
    : InputStream(
              QAndroidJniObject("java/io/ByteArrayInputStream", "([B)V", toArray(bytes).object()))
{
}

ByteArrayOutputStream::ByteArrayOutputStream()
    : OutputStream(QAndroidJniObject("java/io/ByteArrayOutputStream"))
{
    handleExceptions();
}

QByteArray ByteArrayOutputStream::toByteArray() const
{
    const QAndroidJniObject wrapper = callObjectMethod<jbyteArray>("toByteArray");

    if (!handleExceptions())
        return QByteArray();

    return fromArray(static_cast<jbyteArray>(wrapper.object()));
}

int InputStream::read() const
{
    return handleExceptions(callMethod<int>("read"), -1);
}

bool InputStream::readAll(QByteArray &out, QString *errorString) const
{
    QAndroidJniEnvironment env;
    const jobject obj = object();
    const jclass cls = env->GetObjectClass(obj);
    const jmethodID readMethod = env->GetMethodID(cls, "read", "()I");
    env->DeleteLocalRef(cls);
    Q_ASSERT(readMethod);

    out.clear();
    while (true) {
        const jint nextByte = env->CallIntMethod(obj, readMethod);
        if (env->ExceptionCheck()) {
            const jthrowable exception = env->ExceptionOccurred();
            env->ExceptionClear();
            if (errorString && exception) {
                *errorString = QAndroidJniObject(exception).callObjectMethod<jstring>("toString").toString();
                env->DeleteLocalRef(exception);
            }
            return false;
        }
        if (nextByte == -1)
            break;
        out.append(static_cast<char>(nextByte));
    }
    return true;
}

bool OutputStream::write(const QByteArray &bytes) const
{
    callMethod<void>("write", "([B)V", toArray(bytes).object());
    return handleExceptions();
}

bool OutputStream::close() const
{
    QAndroidJniEnvironment env;
    const jclass cls = env->GetObjectClass(object());
    const jmethodID closeMethod = env->GetMethodID(cls, "close", "()V");
    env->DeleteLocalRef(cls);
    Q_ASSERT(closeMethod);
    env->CallVoidMethod(object(), closeMethod);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

bool OutputStream::flush() const
{
    callMethod<void>("flush");
    return handleExceptions();
}

Cipher Cipher::getInstance(const QString &transformation)
{
    return handleExceptions(callStaticObjectMethod("javax/crypto/Cipher", "getInstance",
                                                   "(Ljava/lang/String;)Ljavax/crypto/Cipher;",
                                                   fromString(transformation).object()));
}

bool Cipher::init(int opMode, const Key &key) const
{
    callMethod<void>("init", "(ILjava/security/Key;)V", opMode, key.object());
    return handleExceptions();
}

bool Cipher::init(int opMode, const Key &key,
                  const java::security::spec::AlgorithmParameterSpec &params) const
{
    callMethod<void>("init",
                     "(ILjava/security/Key;Ljava/security/spec/AlgorithmParameterSpec;)V",
                     opMode, key.object(), params.object());
    return handleExceptions();
}

bool Cipher::doFinal(const QByteArray &input, QByteArray &output, QString *errorString) const
{
    QAndroidJniEnvironment env;
    const jobject obj = object();
    const jclass cls = env->GetObjectClass(obj);
    const jmethodID method = env->GetMethodID(cls, "doFinal", "([B)[B");
    env->DeleteLocalRef(cls);
    Q_ASSERT(method);
    const jobject resultObj =
            env->CallObjectMethod(obj, method, toArray(input).object());
    if (env->ExceptionCheck()) {
        const jthrowable exception = env->ExceptionOccurred();
        env->ExceptionClear();
        if (errorString && exception) {
            *errorString = QAndroidJniObject(exception)
                                   .callObjectMethod<jstring>("toString")
                                   .toString();
            env->DeleteLocalRef(exception);
        }
        if (resultObj)
            env->DeleteLocalRef(resultObj);
        return false;
    }
    output = fromArray(static_cast<jbyteArray>(resultObj));
    env->DeleteLocalRef(resultObj);
    return true;
}

SecureRandom::SecureRandom()
    : Object(QAndroidJniObject("java/security/SecureRandom"))
{
    handleExceptions();
}

bool SecureRandom::nextBytes(QByteArray &bytes) const
{
    QAndroidJniEnvironment env;
    const jsize size = static_cast<jsize>(bytes.size());
    const jbyteArray array = env->NewByteArray(size);
    if (!array)
        return false;
    const jobject obj = object();
    const jclass cls = env->GetObjectClass(obj);
    const jmethodID method = env->GetMethodID(cls, "nextBytes", "([B)V");
    env->DeleteLocalRef(cls);
    Q_ASSERT(method);
    env->CallVoidMethod(obj, method, array);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        env->DeleteLocalRef(array);
        return false;
    }
    bytes = fromArray(array);
    env->DeleteLocalRef(array);
    return true;
}

SecretKeySpec::SecretKeySpec(const QByteArray &key, const QString &algorithm)
    : Key(QAndroidJniObject("javax/crypto/spec/SecretKeySpec",
                            "([BLjava/lang/String;)V",
                            toArray(key).object(), fromString(algorithm).object()))
{
    handleExceptions();
}

GCMParameterSpec::GCMParameterSpec(int tLen, const QByteArray &iv)
    : AlgorithmParameterSpec(QAndroidJniObject("javax/crypto/spec/GCMParameterSpec",
                                               "(I[B)V",
                                               static_cast<jint>(tLen), toArray(iv).object()))
{
    handleExceptions();
}

CipherOutputStream::CipherOutputStream(const OutputStream &stream, const Cipher &cipher)
    : FilterOutputStream(QAndroidJniObject("javax/crypto/CipherOutputStream",
                                           "(Ljava/io/OutputStream;Ljavax/crypto/Cipher;)V",
                                           stream.object(), cipher.object()))
{
    handleExceptions();
}

CipherInputStream::CipherInputStream(const InputStream &stream, const Cipher &cipher)
    : FilterInputStream(QAndroidJniObject("javax/crypto/CipherInputStream",
                                          "(Ljava/io/InputStream;Ljavax/crypto/Cipher;)V",
                                          stream.object(), cipher.object()))
{
    handleExceptions();
}
