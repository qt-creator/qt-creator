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

#include "sshparameters.h"

#include "sshsettings.h"

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDebug>

#include <memory>

using namespace Utils;

namespace ProjectExplorer {

SshParameters::SshParameters()
{
    url.setPort(0);
}

QStringList SshParameters::connectionOptions(const FilePath &binary) const
{
    QString hostKeyCheckingString;
    switch (hostKeyCheckingMode) {
    case SshHostKeyCheckingNone:
    case SshHostKeyCheckingAllowNoMatch:
        // There is "accept-new" as well, but only since 7.6.
        hostKeyCheckingString = "no";
        break;
    case SshHostKeyCheckingStrict:
        hostKeyCheckingString = "yes";
        break;
    }

    QStringList args{"-o", "StrictHostKeyChecking=" + hostKeyCheckingString,
                     "-o", "Port=" + QString::number(port())};

    if (!userName().isEmpty())
        args.append({"-o", "User=" + userName()});

    const bool keyOnly = authenticationType ==
            SshParameters::AuthenticationTypeSpecificKey;
    if (keyOnly) {
        args << "-o" << "IdentitiesOnly=yes";
        args << "-i" << privateKeyFile.path();
    }
    if (keyOnly || SshSettings::askpassFilePath().isEmpty())
        args << "-o" << "BatchMode=yes";

    bool useTimeout = timeout != 0;
    if (useTimeout && HostOsInfo::isWindowsHost()
            && binary.toString().toLower().contains("/system32/")) {
        useTimeout = false;
    }
    if (useTimeout)
        args << "-o" << ("ConnectTimeout=" + QString::number(timeout));

    return args;
}

bool SshParameters::setupSshEnvironment(QtcProcess *process)
{
    Environment env = process->controlEnvironment();
    if (env.size() == 0)
        env = Environment::systemEnvironment();
    const bool hasDisplay = env.hasKey("DISPLAY") && (env.value("DISPLAY") != QString(":0"));
    if (SshSettings::askpassFilePath().exists()) {
        env.set("SSH_ASKPASS", SshSettings::askpassFilePath().toUserOutput());

        // OpenSSH only uses the askpass program if DISPLAY is set, regardless of the platform.
        if (!env.hasKey("DISPLAY"))
            env.set("DISPLAY", ":0");
    }
    process->setEnvironment(env);

    // Otherwise, ssh will ignore SSH_ASKPASS and read from /dev/tty directly.
    process->setDisableUnixTerminal();
    return hasDisplay;
}


static inline bool equals(const SshParameters &p1, const SshParameters &p2)
{
    return p1.url == p2.url
            && p1.authenticationType == p2.authenticationType
            && p1.privateKeyFile == p2.privateKeyFile
            && p1.hostKeyCheckingMode == p2.hostKeyCheckingMode
            && p1.x11DisplayName == p2.x11DisplayName
            && p1.timeout == p2.timeout;
}

bool operator==(const SshParameters &p1, const SshParameters &p2)
{
    return equals(p1, p2);
}

bool operator!=(const SshParameters &p1, const SshParameters &p2)
{
    return !equals(p1, p2);
}

#ifdef WITH_TESTS
namespace SshTest {
const QString getHostFromEnvironment()
{
    const QString host = QString::fromLocal8Bit(qgetenv("QTC_SSH_TEST_HOST"));
    if (host.isEmpty() && qEnvironmentVariableIsSet("QTC_SSH_TEST_DEFAULTS"))
        return QString("127.0.0.1");
    return host;
}

quint16 getPortFromEnvironment()
{
    const int port = qEnvironmentVariableIntValue("QTC_SSH_TEST_PORT");
    return port != 0 ? quint16(port) : 22;
}

const QString getUserFromEnvironment()
{
    return QString::fromLocal8Bit(qgetenv("QTC_SSH_TEST_USER"));
}

const QString getKeyFileFromEnvironment()
{
    const FilePath defaultKeyFile = FileUtils::homePath() / ".ssh/id_rsa";
    const QString keyFile = QString::fromLocal8Bit(qgetenv("QTC_SSH_TEST_KEYFILE"));
    if (keyFile.isEmpty()) {
        if (qEnvironmentVariableIsSet("QTC_SSH_TEST_DEFAULTS"))
            return defaultKeyFile.toString();
    }
    return keyFile;
}

const QString userAtHost()
{
    QString userMidFix = getUserFromEnvironment();
    if (!userMidFix.isEmpty())
        userMidFix.append('@');
    return userMidFix + getHostFromEnvironment();
}

SshParameters getParameters()
{
    SshParameters params;
    if (!qEnvironmentVariableIsSet("QTC_SSH_TEST_DEFAULTS")) {
        params.setUserName(getUserFromEnvironment());
        params.privateKeyFile = FilePath::fromUserInput(getKeyFileFromEnvironment());
    }
    params.setHost(getHostFromEnvironment());
    params.setPort(getPortFromEnvironment());
    params.timeout = 10;
    params.authenticationType = !params.privateKeyFile.isEmpty()
            ? SshParameters::AuthenticationTypeSpecificKey
            : SshParameters::AuthenticationTypeAll;
    return params;
}

bool checkParameters(const SshParameters &params)
{
    if (qEnvironmentVariableIsSet("QTC_SSH_TEST_DEFAULTS"))
        return true;
    if (params.host().isEmpty()) {
        qWarning("No hostname provided. Set QTC_SSH_TEST_HOST.");
        return false;
    }
    if (params.userName().isEmpty())
        qWarning("No user name provided - test may fail with empty default. Set QTC_SSH_TEST_USER.");
    if (params.privateKeyFile.isEmpty()) {
        qWarning("No key file provided. Set QTC_SSH_TEST_KEYFILE.");
        return false;
    }
    return true;
}

void printSetupHelp()
{
    qInfo() << "\n\n"
               "In order to run this test properly it requires some setup (example for fedora):\n"
               "1. Run a server on the host to connect to:\n"
               "   systemctl start sshd\n"
               "2. Create your own ssh key (needed only once). For fedora it needs ecdsa type:\n"
               "   ssh-keygen -t ecdsa\n"
               "3. Make your public key known to the server (needed only once):\n"
               "   ssh-copy-id -i [full path to your public key] [user@host]\n"
               "4. Set the env variables before executing test:\n"
               "   QTC_SSH_TEST_HOST=127.0.0.1\n"
               "   QTC_SSH_TEST_KEYFILE=[full path to your private key]\n"
               "   QTC_SSH_TEST_USER=[your user name]\n";
}

} // namespace SshTest
#endif

} // namespace ProjectExplorer
