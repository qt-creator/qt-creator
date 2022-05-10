/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "sshconnection.h"

#include "sftptransfer.h"
#include "sshlogging_p.h"
#include "sshsettings.h"

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QByteArrayList>
#include <QTemporaryDir>
#include <QTimer>

#include <memory>

/*!
    \class QSsh::SshConnection

    \brief The SshConnection class provides an SSH connection via an OpenSSH client
           running in master mode.

    It operates asynchronously (non-blocking) and is not thread-safe.

    If connection sharing is turned off, the class operates as a simple factory
    for processes etc and "connecting" always succeeds. The actual connection
    is then established later, e.g. when starting the remote process.

*/

namespace QSsh {
using namespace Internal;
using namespace Utils;

SshConnectionParameters::SshConnectionParameters()
{
    url.setPort(0);
}

QStringList SshConnectionParameters::connectionOptions(const FilePath &binary) const
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
            SshConnectionParameters::AuthenticationTypeSpecificKey;
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

bool SshConnectionParameters::setupSshEnvironment(QtcProcess *process)
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


static inline bool equals(const SshConnectionParameters &p1, const SshConnectionParameters &p2)
{
    return p1.url == p2.url
            && p1.authenticationType == p2.authenticationType
            && p1.privateKeyFile == p2.privateKeyFile
            && p1.hostKeyCheckingMode == p2.hostKeyCheckingMode
            && p1.x11DisplayName == p2.x11DisplayName
            && p1.timeout == p2.timeout;
}

bool operator==(const SshConnectionParameters &p1, const SshConnectionParameters &p2)
{
    return equals(p1, p2);
}

bool operator!=(const SshConnectionParameters &p1, const SshConnectionParameters &p2)
{
    return !equals(p1, p2);
}

struct SshConnection::SshConnectionPrivate
{
    SshConnectionPrivate(const SshConnectionParameters &sshParameters)
        : connParams(sshParameters)
    {
        SshConnectionParameters::setupSshEnvironment(&masterProcess);
    }

    QString fullProcessError()
    {
        QString error;
        if (masterProcess.exitStatus() != QProcess::NormalExit)
            error = masterProcess.errorString();
        const QByteArray stdErr = masterProcess.readAllStandardError();
        if (!stdErr.isEmpty()) {
            if (!error.isEmpty())
                error.append('\n');
            error.append(QString::fromLocal8Bit(stdErr));
        }
        return error;
    }

    QString socketFilePath() const
    {
        QTC_ASSERT(masterSocketDir, return QString());
        return masterSocketDir->path() + "/cs";
    }

    QStringList connectionOptions(const FilePath &binary) const
    {
        QStringList options = connParams.connectionOptions(binary);
        if (sharingEnabled)
            options << "-o" << ("ControlPath=" + socketFilePath());
        return options;
    }

    QStringList connectionArgs(const FilePath &binary) const
    {
        return connectionOptions(binary) << connParams.host();
    }

    const SshConnectionParameters connParams;
    QtcProcess masterProcess;
    QString errorString;
    std::unique_ptr<QTemporaryDir> masterSocketDir;
    State state = Unconnected;
    const bool sharingEnabled = SshSettings::connectionSharingEnabled();
};


SshConnection::SshConnection(const SshConnectionParameters &serverInfo, QObject *parent)
    : QObject(parent), d(new SshConnectionPrivate(serverInfo))
{
    qRegisterMetaType<QSsh::SftpFileInfo>("QSsh::SftpFileInfo");
    qRegisterMetaType<QList <QSsh::SftpFileInfo> >("QList<QSsh::SftpFileInfo>");
    connect(&d->masterProcess, &QtcProcess::readyReadStandardOutput, [this] {
        const QByteArray reply = d->masterProcess.readAllStandardOutput();
        if (reply == "\n")
            emitConnected();
    });
    connect(&d->masterProcess, &QtcProcess::done, this, [this] {
        if (d->masterProcess.error() == QProcess::FailedToStart) {
            emitError(tr("Cannot establish SSH connection: Control process failed to start: %1")
                      .arg(d->fullProcessError()));
            return;
        }
        if (d->state == Disconnecting) {
            emitDisconnected();
            return;
        }
        const QString procError = d->fullProcessError();
        QString errorMsg = tr("SSH connection failure.");
        if (!procError.isEmpty())
            errorMsg.append('\n').append(procError);
        emitError(errorMsg);
    });
    if (!d->connParams.x11DisplayName.isEmpty()) {
        Environment env = d->masterProcess.environment();
        env.set("DISPLAY", d->connParams.x11DisplayName);
        d->masterProcess.setEnvironment(env);
    }
}

void SshConnection::connectToHost()
{
    d->state = Connecting;
    QTimer::singleShot(0, this, &SshConnection::doConnectToHost);
}

void SshConnection::disconnectFromHost()
{
    switch (d->state) {
    case Connecting:
    case Connected:
        if (!d->sharingEnabled) {
            QTimer::singleShot(0, this, &SshConnection::emitDisconnected);
            return;
        }
        d->state = Disconnecting;
        if (HostOsInfo::isWindowsHost())
            d->masterProcess.kill();
        else
            d->masterProcess.terminate();
        break;
    case Unconnected:
    case Disconnecting:
        break;
    }
}

SshConnection::State SshConnection::state() const
{
    return d->state;
}

QString SshConnection::errorString() const
{
    return d->errorString;
}

SshConnectionParameters SshConnection::connectionParameters() const
{
    return d->connParams;
}

QStringList SshConnection::connectionOptions(const FilePath &binary) const
{
    return d->connectionOptions(binary);
}

bool SshConnection::sharingEnabled() const
{
    return d->sharingEnabled;
}

SshConnection::~SshConnection()
{
    disconnect();
    disconnectFromHost();
    delete d;
}

SftpTransferPtr SshConnection::createUpload(const FilesToTransfer &files)
{
    return setupTransfer(files, Internal::FileTransferType::Upload);
}

SftpTransferPtr SshConnection::createDownload(const FilesToTransfer &files)
{
    return setupTransfer(files, Internal::FileTransferType::Download);
}

void SshConnection::doConnectToHost()
{
    if (d->state != Connecting)
        return;
    const FilePath sshBinary = SshSettings::sshFilePath();
    if (!sshBinary.exists()) {
        emitError(tr("Cannot establish SSH connection: ssh binary \"%1\" does not exist.")
                  .arg(sshBinary.toUserOutput()));
        return;
    }
    if (!d->sharingEnabled) {
        emitConnected();
        return;
    }
    d->masterSocketDir.reset(new QTemporaryDir);
    if (!d->masterSocketDir->isValid()) {
        emitError(tr("Cannot establish SSH connection: Failed to create temporary "
                     "directory for control socket: %1")
                  .arg(d->masterSocketDir->errorString()));
        return;
    }
    QStringList args = QStringList{"-M", "-N", "-o", "ControlPersist=no",
            "-o", "PermitLocalCommand=yes", // Enable local command
            "-o", "LocalCommand=echo"}      // Local command is executed after successfully
                                            // connecting to the server. "echo" will print "\n"
                                            // on the process output if everything went fine.
            << d->connectionArgs(sshBinary);
    if (!d->connParams.x11DisplayName.isEmpty())
        args.prepend("-X");
    qCDebug(sshLog) << "establishing connection:" << sshBinary.toUserOutput() << args;
    d->masterProcess.setCommand(CommandLine(sshBinary, args));
    d->masterProcess.start();
}

void SshConnection::emitError(const QString &reason)
{
    const State oldState = d->state;
    d->state = Unconnected;
    d->errorString = reason;
    emit errorOccurred();
    if (oldState == Connected)
        emitDisconnected();
}

void SshConnection::emitConnected()
{
    d->state = Connected;
    emit connected();
}

void SshConnection::emitDisconnected()
{
    d->state = Unconnected;
    emit disconnected();
}

SftpTransferPtr SshConnection::setupTransfer(const FilesToTransfer &files,
                                             Internal::FileTransferType type)
{
    QTC_ASSERT(state() == Connected, return SftpTransferPtr());
    return SftpTransferPtr(new SftpTransfer(files, type,
                                            d->connectionArgs(SshSettings::sftpFilePath())));
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

SshConnectionParameters getParameters()
{
    SshConnectionParameters params;
    if (!qEnvironmentVariableIsSet("QTC_SSH_TEST_DEFAULTS")) {
        params.setUserName(getUserFromEnvironment());
        params.privateKeyFile = Utils::FilePath::fromUserInput(getKeyFileFromEnvironment());
    }
    params.setHost(getHostFromEnvironment());
    params.setPort(getPortFromEnvironment());
    params.timeout = 10;
    params.authenticationType = !params.privateKeyFile.isEmpty()
            ? QSsh::SshConnectionParameters::AuthenticationTypeSpecificKey
            : QSsh::SshConnectionParameters::AuthenticationTypeAll;
    return params;
}

bool checkParameters(const QSsh::SshConnectionParameters &params)
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

} // namespace QSsh
