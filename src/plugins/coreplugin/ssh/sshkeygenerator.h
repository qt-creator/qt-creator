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

#ifndef SSHKEYGENERATOR_H
#define SSHKEYGENERATOR_H

#include <coreplugin/core_global.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QSharedPointer>

namespace Botan {
    class Private_Key;
}

namespace Core {

class CORE_EXPORT SshKeyGenerator
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
    bool generateOpenSslKeys(const KeyPtr &key, KeyType type);

    QString m_error;
    QByteArray m_publicKey;
    QByteArray m_privateKey;
    KeyType m_type;
    PrivateKeyFormat m_format;
};

} // namespace Core

#endif // SSHKEYGENERATOR_H
