// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sshparameters.h"

#include "sshsettings.h"

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QDebug>

#include <memory>

using namespace Utils;

namespace ProjectExplorer {

SshParameters::SshParameters() = default;

QString SshParameters::userAtHost() const
{
    QString res;
    if (!m_userName.isEmpty())
        res = m_userName + '@';
    res += m_host;
    return res;
}

QString SshParameters::userAtHostAndPort() const
{
    QString res = SshParameters::userAtHost();
    if (m_port != 22)
        res += QString(":%1").arg(m_port);
    return res;
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
        args << "-o" << "User=" + userName();

    const bool keyOnly = authenticationType == SshParameters::AuthenticationTypeSpecificKey;
    if (keyOnly)
        args << "-o" << "IdentitiesOnly=yes" << "-i" << privateKeyFile.path();

    const QString batchModeEnabled = (keyOnly || SshSettings::askpassFilePath().isEmpty())
            ? QLatin1String("yes") : QLatin1String("no");
    args << "-o" << "BatchMode=" + batchModeEnabled;

    const bool isWindows = HostOsInfo::isWindowsHost()
            && binary.toString().toLower().contains("/system32/");
    const bool useTimeout = (timeout != 0) && !isWindows;
    if (useTimeout)
        args << "-o" << "ConnectTimeout=" + QString::number(timeout);

    return args;
}

bool SshParameters::setupSshEnvironment(Process *process)
{
    Environment env = process->controlEnvironment();
    if (!env.hasChanges())
        env = Environment::systemEnvironment();
    const bool hasDisplay = env.hasKey("DISPLAY") && (env.value("DISPLAY") != QString(":0"));
    if (SshSettings::askpassFilePath().exists()) {
        env.set("SSH_ASKPASS", SshSettings::askpassFilePath().toUserOutput());
        env.set("SSH_ASKPASS_REQUIRE", "force");

        // OpenSSH only uses the askpass program if DISPLAY is set, regardless of the platform.
        if (!env.hasKey("DISPLAY"))
            env.set("DISPLAY", ":0");
    }
    process->setEnvironment(env);

    // Otherwise, ssh will ignore SSH_ASKPASS and read from /dev/tty directly.
    process->setDisableUnixTerminal();
    return hasDisplay;
}

bool operator==(const SshParameters &p1, const SshParameters &p2)
{
    return p1.m_host == p2.m_host
            && p1.m_port == p2.m_port
            && p1.m_userName == p2.m_userName
            && p1.authenticationType == p2.authenticationType
            && p1.privateKeyFile == p2.privateKeyFile
            && p1.hostKeyCheckingMode == p2.hostKeyCheckingMode
            && p1.x11DisplayName == p2.x11DisplayName
            && p1.timeout == p2.timeout;
}

#ifdef WITH_TESTS
namespace SshTest {
const QString getHostFromEnvironment()
{
    const QString host = qtcEnvironmentVariable("QTC_SSH_TEST_HOST");
    if (host.isEmpty() && qtcEnvironmentVariableIsSet("QTC_SSH_TEST_DEFAULTS"))
        return QString("127.0.0.1");
    return host;
}

quint16 getPortFromEnvironment()
{
    const int port = qtcEnvironmentVariableIntValue("QTC_SSH_TEST_PORT");
    return port != 0 ? quint16(port) : 22;
}

const QString getUserFromEnvironment()
{
    return qtcEnvironmentVariable("QTC_SSH_TEST_USER");
}

const QString getKeyFileFromEnvironment()
{
    const FilePath defaultKeyFile = FileUtils::homePath() / ".ssh/id_rsa";
    const QString keyFile = qtcEnvironmentVariable("QTC_SSH_TEST_KEYFILE");
    if (keyFile.isEmpty()) {
        if (qtcEnvironmentVariableIsSet("QTC_SSH_TEST_DEFAULTS"))
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

const QString userAtHostAndPort()
{
    QString res = userAtHost();
    const int port = getPortFromEnvironment();
    if (port != 22)
        res += QString(":%1").arg(port);
    return res;
}

SshParameters getParameters()
{
    SshParameters params;
    if (!qtcEnvironmentVariableIsSet("QTC_SSH_TEST_DEFAULTS")) {
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
    if (qtcEnvironmentVariableIsSet("QTC_SSH_TEST_DEFAULTS"))
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
