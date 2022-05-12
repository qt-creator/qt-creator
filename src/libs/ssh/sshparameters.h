/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "ssh_global.h"

#include <utils/filepath.h>

#include <QUrl>

namespace Utils { class QtcProcess; }

namespace QSsh {

enum SshHostKeyCheckingMode {
    SshHostKeyCheckingNone,
    SshHostKeyCheckingStrict,
    SshHostKeyCheckingAllowNoMatch,
};

class QSSH_EXPORT SshParameters
{
public:
    enum AuthenticationType {
        AuthenticationTypeAll,
        AuthenticationTypeSpecificKey,
    };

    SshParameters();

    QString host() const { return url.host(); }
    quint16 port() const { return url.port(); }
    QString userName() const { return url.userName(); }
    QString userAtHost() const { return userName().isEmpty() ? host() : userName() + '@' + host(); }
    void setHost(const QString &host) { url.setHost(host); }
    void setPort(int port) { url.setPort(port); }
    void setUserName(const QString &name) { url.setUserName(name); }

    QStringList connectionOptions(const Utils::FilePath &binary) const;

    QUrl url;
    Utils::FilePath privateKeyFile;
    QString x11DisplayName;
    int timeout = 0; // In seconds.
    AuthenticationType authenticationType = AuthenticationTypeAll;
    SshHostKeyCheckingMode hostKeyCheckingMode = SshHostKeyCheckingAllowNoMatch;

    static bool setupSshEnvironment(Utils::QtcProcess *process);
};

QSSH_EXPORT bool operator==(const SshParameters &p1, const SshParameters &p2);
QSSH_EXPORT bool operator!=(const SshParameters &p1, const SshParameters &p2);

#ifdef WITH_TESTS
namespace SshTest {
const QString QSSH_EXPORT getHostFromEnvironment();
quint16 QSSH_EXPORT getPortFromEnvironment();
const QString QSSH_EXPORT getUserFromEnvironment();
const QString QSSH_EXPORT getKeyFileFromEnvironment();
const QSSH_EXPORT QString userAtHost();
SshParameters QSSH_EXPORT getParameters();
bool QSSH_EXPORT checkParameters(const SshParameters &params);
void QSSH_EXPORT printSetupHelp();
} // namespace SshTest
#endif

} // namespace QSsh

Q_DECLARE_METATYPE(QSsh::SshParameters::AuthenticationType)
