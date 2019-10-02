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

#include "sftpsession.h"
#include "sftptransfer.h"
#include "sshlogging_p.h"
#include "sshprocess.h"
#include "sshremoteprocess.h"
#include "sshsettings.h"

#include <utils/filesystemwatcher.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QByteArrayList>
#include <QDir>
#include <QFileInfo>
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
        QString hostKeyCheckingString;
        switch (connParams.hostKeyCheckingMode) {
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
                    "-o", "User=" + connParams.userName(),
                    "-o", "Port=" + QString::number(connParams.port())};
        const bool keyOnly = connParams.authenticationType ==
                SshConnectionParameters::AuthenticationTypeSpecificKey;
        if (keyOnly)
            args << "-i" << connParams.privateKeyFile;
        if (keyOnly || SshSettings::askpassFilePath().isEmpty())
            args << "-o" << "BatchMode=yes";
        if (sharingEnabled)
            args << "-o" << ("ControlPath=" + socketFilePath());
        bool useTimeout = connParams.timeout != 0;
        if (useTimeout && HostOsInfo::isWindowsHost()
                && binary.toString().toLower().contains("/system32/")) {
            useTimeout = false;
        }
        if (useTimeout)
            args << "-o" << ("ConnectTimeout=" + QString::number(connParams.timeout));
        return args;
    }

    QStringList connectionArgs(const FilePath &binary) const
    {
        return connectionOptions(binary) << connParams.host();
    }

    SshConnectionParameters connParams;
    SshConnectionInfo connInfo;
    SshProcess masterProcess;
    QString errorString;
    std::unique_ptr<QTemporaryDir> masterSocketDir;
    State state = Unconnected;
    const bool sharingEnabled = SshSettings::connectionSharingEnabled();
};


SshConnection::SshConnection(const SshConnectionParameters &serverInfo, QObject *parent)
    : QObject(parent), d(new SshConnectionPrivate)
{
    qRegisterMetaType<QSsh::SftpFileInfo>("QSsh::SftpFileInfo");
    qRegisterMetaType<QList <QSsh::SftpFileInfo> >("QList<QSsh::SftpFileInfo>");
    d->connParams = serverInfo;
    connect(&d->masterProcess, &QProcess::started, [this] {
        QFileInfo socketInfo(d->socketFilePath());
        if (socketInfo.exists()) {
            emitConnected();
            return;
        }
        auto * const socketWatcher = new FileSystemWatcher(this);
        auto * const socketWatcherTimer = new QTimer(this);
        const auto socketFileChecker = [this, socketWatcher, socketWatcherTimer] {
            if (!QFileInfo::exists(d->socketFilePath()))
                return;
            socketWatcher->disconnect();
            socketWatcher->deleteLater();
            socketWatcherTimer->disconnect();
            socketWatcherTimer->stop();
            socketWatcherTimer->deleteLater();
            emitConnected();
        };
        connect(socketWatcher, &FileSystemWatcher::directoryChanged, socketFileChecker);
        socketWatcher->addDirectory(socketInfo.path(), FileSystemWatcher::WatchAllChanges);
        if (HostOsInfo::isMacHost()) {
            // QTBUG-72455
            socketWatcherTimer->setInterval(1000);
            connect(socketWatcherTimer, &QTimer::timeout, socketFileChecker);
            socketWatcherTimer->start();
        }
    });
    connect(&d->masterProcess, &QProcess::errorOccurred, [this] (QProcess::ProcessError error) {
        switch (error) {
        case QProcess::FailedToStart:
            emitError(tr("Cannot establish SSH connection: Control process failed to start: %1")
                      .arg(d->fullProcessError()));
            break;
        case QProcess::Crashed: // Handled by finished() handler.
        case QProcess::Timedout:
        case QProcess::ReadError:
        case QProcess::WriteError:
        case QProcess::UnknownError:
            break; // Cannot happen.
        }
    });
    connect(&d->masterProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [this] {
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
        QProcessEnvironment env = d->masterProcess.processEnvironment();
        env.insert("DISPLAY", d->connParams.x11DisplayName);
        d->masterProcess.setProcessEnvironment(env);
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

SshConnectionInfo SshConnection::connectionInfo() const
{
    QTC_ASSERT(state() == Connected, return SshConnectionInfo());
    if (d->connInfo.isValid())
        return d->connInfo;
    QProcess p;
    p.start(SshSettings::sshFilePath().toString(), d->connectionArgs(SshSettings::sshFilePath())
            << "echo" << "-n" << "$SSH_CLIENT");
    if (!p.waitForStarted() || !p.waitForFinished()) {
        qCWarning(Internal::sshLog) << "failed to retrieve connection info:" << p.errorString();
        return SshConnectionInfo();
    }
    const QByteArrayList data = p.readAllStandardOutput().split(' ');
    if (data.size() != 3) {
        qCWarning(Internal::sshLog) << "failed to retrieve connection info: unexpected output";
        return SshConnectionInfo();
    }
    d->connInfo.localPort = data.at(1).toInt();
    if (d->connInfo.localPort == 0) {
        qCWarning(Internal::sshLog) << "failed to retrieve connection info: unexpected output";
        return SshConnectionInfo();
    }
    if (!d->connInfo.localAddress.setAddress(QString::fromLatin1(data.first()))) {
        qCWarning(Internal::sshLog) << "failed to retrieve connection info: unexpected output";
        return SshConnectionInfo();
    }
    d->connInfo.peerPort = d->connParams.port();
    d->connInfo.peerAddress.setAddress(d->connParams.host());
    return d->connInfo;
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

SshRemoteProcessPtr SshConnection::createRemoteProcess(const QString &command)
{
    QTC_ASSERT(state() == Connected, return SshRemoteProcessPtr());
    return SshRemoteProcessPtr(new SshRemoteProcess(command,
                                                    d->connectionArgs(SshSettings::sshFilePath())));
}

SshRemoteProcessPtr SshConnection::createRemoteShell()
{
    return createRemoteProcess({});
}

SftpTransferPtr SshConnection::createUpload(const FilesToTransfer &files,
                                            FileTransferErrorHandling errorHandlingMode)
{
    return setupTransfer(files, Internal::FileTransferType::Upload, errorHandlingMode);
}

SftpTransferPtr SshConnection::createDownload(const FilesToTransfer &files,
                                              FileTransferErrorHandling errorHandlingMode)
{
    return setupTransfer(files, Internal::FileTransferType::Download, errorHandlingMode);
}

SftpSessionPtr SshConnection::createSftpSession()
{
    QTC_ASSERT(state() == Connected, return SftpSessionPtr());
    return SftpSessionPtr(new SftpSession(d->connectionArgs(SshSettings::sftpFilePath())));
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
    QStringList args = QStringList{"-M", "-N", "-o", "ControlPersist=no"}
            << d->connectionArgs(sshBinary);
    if (!d->connParams.x11DisplayName.isEmpty())
        args.prepend("-X");
    qCDebug(sshLog) << "establishing connection:" << sshBinary.toUserOutput() << args;
    d->masterProcess.start(sshBinary.toString(), args);
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

SftpTransferPtr SshConnection::setupTransfer(
        const FilesToTransfer &files, Internal::FileTransferType type,
        FileTransferErrorHandling errorHandlingMode)
{
    QTC_ASSERT(state() == Connected, return SftpTransferPtr());
    return SftpTransferPtr(new SftpTransfer(files, type, errorHandlingMode,
                                            d->connectionArgs(SshSettings::sftpFilePath())));
}

} // namespace QSsh
