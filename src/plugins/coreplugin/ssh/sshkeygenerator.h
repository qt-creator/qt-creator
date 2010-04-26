#ifndef SSHKEYGENERATOR_H
#define SSHKEYGENERATOR_H

#include <coreplugin/core_global.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QPair>

namespace Core {

class CORE_EXPORT SshKeyGenerator
{
    Q_DECLARE_TR_FUNCTIONS(SshKeyGenerator)
public:
    enum KeyType { Rsa, Dsa };

    SshKeyGenerator();
    bool generateKeys(KeyType type, const QString &id, int keySize);
    QString error() const { return m_error; }
    QString privateKey() const { return m_privateKey; }
    QString publicKey() const { return m_publicKey; }
    KeyType type() const { return m_type; }

private:
    QString m_error;
    QString m_publicKey;
    QString m_privateKey;
    KeyType m_type;
};

} // namespace Core

#endif // SSHKEYGENERATOR_H
