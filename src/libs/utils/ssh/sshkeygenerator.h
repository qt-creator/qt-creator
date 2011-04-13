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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SSHKEYGENERATOR_H
#define SSHKEYGENERATOR_H

#include <utils/utils_global.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QSharedPointer>

namespace Botan {
    class Private_Key;
}

namespace Utils {

class QTCREATOR_UTILS_EXPORT SshKeyGenerator
{
    Q_DECLARE_TR_FUNCTIONS(SshKeyGenerator)
public:
    enum KeyType { Rsa, Dsa };
    enum PrivateKeyFormat { Pkcs8, OpenSsl };

    SshKeyGenerator();
    bool generateKeys(KeyType type, PrivateKeyFormat format, int keySize);
    QString error() const { return m_error; }
    QByteArray privateKey() const { return m_privateKey; }
    QByteArray publicKey() const { return m_publicKey; }
    KeyType type() const { return m_type; }
    PrivateKeyFormat format() const { return m_format; }

private:
    typedef QSharedPointer<Botan::Private_Key> KeyPtr;

    bool generatePkcs8Keys(const KeyPtr &key);
    void generatePkcs8Key(const KeyPtr &key, bool privateKey);
    bool generateOpenSslKeys(const KeyPtr &key);

    QString m_error;
    QByteArray m_publicKey;
    QByteArray m_privateKey;
    KeyType m_type;
    PrivateKeyFormat m_format;
};

} // namespace Utils

#endif // SSHKEYGENERATOR_H
