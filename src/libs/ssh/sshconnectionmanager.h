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

#ifndef SSHCONNECTIONMANAGER_H
#define SSHCONNECTIONMANAGER_H

#include "ssh_global.h"

#include <QScopedPointer>

namespace QSsh {
class SshConnection;
class SshConnectionParameters;
namespace Internal { class SshConnectionManagerPrivate; }

class QSSH_EXPORT SshConnectionManager
{
    friend class Internal::SshConnectionManagerPrivate;
public:
    static SshConnectionManager &instance();

    SshConnection *acquireConnection(const SshConnectionParameters &sshParams);
    void releaseConnection(SshConnection *connection);
    // Make sure the next acquireConnection with the given parameters will return a new connection.
    void forceNewConnection(const SshConnectionParameters &sshParams);

private:
    explicit SshConnectionManager();
    virtual ~SshConnectionManager();
    SshConnectionManager(const SshConnectionManager &);
    SshConnectionManager &operator=(const SshConnectionManager &);

    const QScopedPointer<Internal::SshConnectionManagerPrivate> d;
};

} // namespace QSsh

#endif // SSHCONNECTIONMANAGER_H
