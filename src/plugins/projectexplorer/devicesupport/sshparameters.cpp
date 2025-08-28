// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sshparameters.h"

#include "../projectexplorertr.h"
#include "sshsettings.h"

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
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
    switch (m_hostKeyCheckingMode) {
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

    const bool keyOnly = m_authenticationType == SshParameters::AuthenticationTypeSpecificKey;
    if (keyOnly && m_privateKeyFile.isReadableFile())
        args << "-o" << "IdentitiesOnly=yes" << "-i" << m_privateKeyFile.path();

    const QString batchModeEnabled = (keyOnly || SshSettings::askpassFilePath().isEmpty())
            ? QLatin1String("yes") : QLatin1String("no");
    args << "-o" << "BatchMode=" + batchModeEnabled;

    const bool isWindows = HostOsInfo::isWindowsHost()
            && binary.toUrlishString().toLower().contains("/system32/");
    const bool useTimeout = (m_timeout != 0) && !isWindows;
    if (useTimeout)
        args << "-o" << "ConnectTimeout=" + QString::number(m_timeout);

    return args;
}

void SshParameters::setupSshEnvironment(Process *process)
{
    Environment env = process->controlEnvironment();
    if (!env.hasChanges())
        env = Environment::systemEnvironment();
    const FilePath askPass = SshSettings::askpassFilePath();
    if (askPass.exists()) {
        if (askPass.fileName().contains("qtc"))
            env = Environment::originalSystemEnvironment();
        env.set("SSH_ASKPASS", askPass.toUserOutput());
        env.set("SSH_ASKPASS_REQUIRE", "force");

        // OpenSSH only uses the askpass program if DISPLAY is set, regardless of the platform.
        if (!env.hasKey("DISPLAY"))
            env.set("DISPLAY", ":0");
    }
    process->setEnvironment(env);

    // Originally, OpenSSH used to ignore the value of SSH_ASKPASS if it had access to a terminal,
    // reading instead from /dev/tty directly.
    // Therefore, we needed to start the ssh process in a new session.
    // Since version 8.4, we can (and do) force the use of SSH_ASKPASS via the SSH_ASKPASS_REQUIRE
    // environment variable. We still create a new session for backward compatibility, but not
    // on macOS, because of QTCREATORBUG-32613.
    if (!HostOsInfo::isMacHost())
        process->setDisableUnixTerminal();
}

bool operator==(const SshParameters &p1, const SshParameters &p2)
{
    return p1.m_host == p2.m_host
            && p1.m_port == p2.m_port
            && p1.m_userName == p2.m_userName
            && p1.m_authenticationType == p2.m_authenticationType
            && p1.m_privateKeyFile == p2.m_privateKeyFile
            && p1.m_hostKeyCheckingMode == p2.m_hostKeyCheckingMode
            && p1.m_x11DisplayName == p2.m_x11DisplayName
            && p1.m_timeout == p2.m_timeout;
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
            return defaultKeyFile.toUrlishString();
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
        params.setPrivateKeyFile(FilePath::fromUserInput(getKeyFileFromEnvironment()));
    }
    params.setHost(getHostFromEnvironment());
    params.setPort(getPortFromEnvironment());
    params.setTimeout(10);
    params.setAuthenticationType(
        !params.privateKeyFile().isEmpty() ? SshParameters::AuthenticationTypeSpecificKey
                                           : SshParameters::AuthenticationTypeAll);
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

void SshParametersAspectContainer::setSshParameters(const SshParameters &params)
{
    QTC_ASSERT(QThread::currentThread() == thread(), return);

    host.setVolatileValue(params.host());
    port.setVolatileValue(params.port());
    userName.setVolatileValue(params.userName());
    privateKeyFile.setVolatileValue(params.privateKeyFile().toUserOutput());
    timeout.setVolatileValue(params.timeout());
    useKeyFile.setVolatileValue(params.authenticationType() == SshParameters::AuthenticationTypeSpecificKey);
    hostKeyCheckingMode.setVolatileValue(params.hostKeyCheckingMode());

    privateKeyFile.setEnabled(
        params.authenticationType() == SshParameters::AuthenticationTypeSpecificKey);

    // This will emit the applied signal which the IDevice uses to update the ssh parameters.
    apply();
}

SshParameters SshParametersAspectContainer::sshParameters() const
{
    QTC_ASSERT(QThread::currentThread() == thread(), return SshParameters());

    SshParameters params;
    params.setHost(host.expandedValue());
    params.setPort(port.value());
    params.setUserName(userName.expandedValue());
    params.setPrivateKeyFile(privateKeyFile.expandedValue());
    params.setTimeout(timeout.value());
    params.setAuthenticationType(
        useKeyFile() ? SshParameters::AuthenticationTypeSpecificKey
                     : SshParameters::AuthenticationTypeAll);
    params.setHostKeyCheckingMode(hostKeyCheckingMode.value());
    return params;
}

SshParametersAspectContainer::SshParametersAspectContainer()
{
    useKeyFile.setDefaultValue(SshParameters::AuthenticationTypeAll);
    useKeyFile.setToolTip(Tr::tr("Enable to specify a private key file to use for authentication, "
                                 "otherwise the default mechanism is used for authentication "
                                 "(password, .sshconfig and the default private key)."));
    useKeyFile.setLabelText(Tr::tr("Use specific key:"));

    hostKeyCheckingMode.setToolTip(Tr::tr("The device's SSH host key checking mode."));
    hostKeyCheckingMode.setLabelText(Tr::tr("Host key check:"));
    hostKeyCheckingMode.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    hostKeyCheckingMode.addOption(Tr::tr("None"), Tr::tr("No host key checking."));
    hostKeyCheckingMode.addOption(Tr::tr("Strict"), Tr::tr("Strict host key checking."));
    hostKeyCheckingMode.addOption(Tr::tr("Allow No Match"), Tr::tr("Allow host key checking."));

    host.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);
    host.setPlaceHolderText(Tr::tr("Host name or IP address"));
    host.setToolTip(Tr::tr("The device's host name or IP address."));
    host.setHistoryCompleter("HostName");
    host.setLabelText(Tr::tr("Host name:"));

    userName.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);
    userName.setPlaceHolderText(Tr::tr("User name"));
    userName.setToolTip(Tr::tr("The device's SSH user name."));
    userName.setHistoryCompleter("UserName");
    userName.setLabelText(Tr::tr("User name:"));

    port.setDefaultValue(22);
    port.setRange(1, 65535);
    port.setToolTip(Tr::tr("The device's SSH port number."));
    port.setLabelText(Tr::tr("SSH port:"));

    privateKeyFile.setPlaceHolderText(Tr::tr("Private key file"));
    privateKeyFile.setToolTip(Tr::tr("The device's private key file."));
    privateKeyFile.setLabelText(Tr::tr("Private key file:"));
    privateKeyFile.setHistoryCompleter("KeyFile");
    privateKeyFile.setEnabler(&useKeyFile);

    timeout.setDefaultValue(10);
    timeout.setLabelText(Tr::tr("Timeout:"));
    timeout.setToolTip(Tr::tr("The device's SSH connection timeout."));
}

} // namespace ProjectExplorer
