#include "sshkeygenerator.h"

#include "ne7sshobject.h"

#include <QtCore/QFile>
#include <QtCore/QTemporaryFile>

#include <ne7ssh.h>

namespace Core {

SshKeyGenerator::SshKeyGenerator()
{
}

bool SshKeyGenerator::generateKeys(KeyType type, const QString &id, int keySize)
{
    QTemporaryFile tmpPubKeyFile;
    QTemporaryFile tmpPrivKeyFile;
    if (!tmpPubKeyFile.open() || !tmpPrivKeyFile.open()) {
        m_error = tr("Error creating temporary files.");
        return false;
    }
    tmpPubKeyFile.setAutoRemove(false);
    tmpPubKeyFile.close();
    tmpPrivKeyFile.close();
    const char * const typeStr = type == Rsa ? "rsa" : "dsa";
    Internal::Ne7SshObject::Ptr ne7Object
        = Internal::Ne7SshObject::instance()->get();
    if (!ne7Object->generateKeyPair(typeStr, id.toUtf8(),
             tmpPrivKeyFile.fileName().toUtf8(),
             tmpPubKeyFile.fileName().toUtf8(), keySize)) {
        // TODO: Race condition on pop() call. Perhaps not use Net7 errors? Or hack API
        m_error = tr("Error generating keys: %1")
                  .arg(ne7Object->errors()->pop());
        return false;
    }

    if (!tmpPubKeyFile.open() || !tmpPrivKeyFile.open()) {
        m_error = tr("Error reading temporary files.");
        return false;
    }

    m_publicKey = tmpPubKeyFile.readAll();
    m_privateKey = tmpPrivKeyFile.readAll();
    if (tmpPubKeyFile.error() != QFile::NoError
        || tmpPrivKeyFile.error() != QFile::NoError) {
        m_error = tr("Error reading temporary files.");
        return false;
    }

    m_type = type;
    return true;
}

} // namespace Core
