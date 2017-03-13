/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "valgrindprocess.h"

#include <ssh/sshconnectionmanager.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDebug>
#include <QEventLoop>

using namespace ProjectExplorer;

namespace Valgrind {

ValgrindProcess::ValgrindProcess(const IDevice::ConstPtr &device,
                                 QObject *parent)
    : QObject(parent), m_device(device)
{
    m_remote.m_connection = 0;
    m_remote.m_error = QProcess::UnknownError;
    m_pid = 0;
}

ValgrindProcess::~ValgrindProcess()
{
    if (m_remote.m_connection)
        QSsh::releaseConnection(m_remote.m_connection);
}

void ValgrindProcess::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    if (isLocal())
        m_localProcess.setProcessChannelMode(mode);
    ///TODO: remote support this by handling the mode internally
}

QString ValgrindProcess::workingDirectory() const
{
    return m_debuggee.workingDirectory;
}

bool ValgrindProcess::isRunning() const
{
    if (isLocal())
        return m_localProcess.isRunning();
    else
        return m_remote.m_process && m_remote.m_process->isRunning();
}

void ValgrindProcess::setValgrindExecutable(const QString &valgrindExecutable)
{
    m_valgrindExecutable = valgrindExecutable;
}

void ValgrindProcess::setDebuggee(const StandardRunnable &debuggee)
{
    m_debuggee = debuggee;
}

void ValgrindProcess::setValgrindArguments(const QStringList &valgrindArguments)
{
    m_valgrindArguments = valgrindArguments;
}

void ValgrindProcess::close()
{
    if (isLocal()) {
        m_localProcess.stop();
    } else {
        QTC_ASSERT(m_remote.m_connection->state() == QSsh::SshConnection::Connected, return);
        if (m_remote.m_process) {
            if (m_pid) {
                const QString killTemplate = QString::fromLatin1("kill -%2 %1" // kill
                                                    ).arg(m_pid);

                const QString niceKill = killTemplate.arg(QLatin1String("SIGTERM"));
                const QString brutalKill = killTemplate.arg(QLatin1String("SIGKILL"));
                const QString remoteCall = niceKill + QLatin1String("; sleep 1; ") + brutalKill;

                QSsh::SshRemoteProcess::Ptr cleanup = m_remote.m_connection->createRemoteProcess(remoteCall.toUtf8());
                cleanup->start();
            }
        }
    }
}

void ValgrindProcess::run(ApplicationLauncher::Mode runMode)
{
    if (isLocal()) {
        connect(&m_localProcess, &ApplicationLauncher::processExited,
                this, &ValgrindProcess::finished);
        connect(&m_localProcess, &ApplicationLauncher::processStarted,
                this, &ValgrindProcess::localProcessStarted);
        connect(&m_localProcess, &ApplicationLauncher::error,
                this, &ValgrindProcess::error);
        connect(&m_localProcess, &ApplicationLauncher::appendMessage,
                this, &ValgrindProcess::processOutput);

        StandardRunnable valgrind;
        valgrind.executable = m_valgrindExecutable;
        valgrind.runMode = runMode;
        valgrind.commandLineArguments = argumentString(Utils::HostOsInfo::hostOs());
        valgrind.workingDirectory = m_debuggee.workingDirectory;
        valgrind.environment = m_debuggee.environment;
        m_localProcess.start(valgrind);

    } else {
        // connect to host and wait for connection
        if (!m_remote.m_connection)
            m_remote.m_connection = QSsh::acquireConnection(m_device->sshParameters());

        if (m_remote.m_connection->state() != QSsh::SshConnection::Connected) {
            connect(m_remote.m_connection, &QSsh::SshConnection::connected,
                    this, &ValgrindProcess::connected);
            connect(m_remote.m_connection, &QSsh::SshConnection::error,
                    this, &ValgrindProcess::handleError);
            if (m_remote.m_connection->state() == QSsh::SshConnection::Unconnected)
                m_remote.m_connection->connectToHost();
        } else {
            connected();
        }
    }
}

QString ValgrindProcess::errorString() const
{
    if (isLocal())
        return m_localProcess.errorString();
    else
        return m_remote.m_errorString;
}

QProcess::ProcessError ValgrindProcess::processError() const
{
    if (isLocal())
        return m_localProcess.processError();
    else
        return m_remote.m_error;
}

void ValgrindProcess::handleError(QSsh::SshError error)
{
    if (!isLocal()) {
        switch (error) {
            case QSsh::SshTimeoutError:
                m_remote.m_error = QProcess::Timedout;
                break;
            default:
                m_remote.m_error = QProcess::FailedToStart;
                break;
        }
    }
    m_remote.m_errorString = m_remote.m_connection->errorString();
    emit this->error(m_remote.m_error);
}

qint64 ValgrindProcess::pid() const
{
    return m_pid;
}

void ValgrindProcess::handleRemoteStderr()
{
    const QString b = QString::fromUtf8(m_remote.m_process->readAllStandardError());
    if (!b.isEmpty())
        emit processOutput(b, Utils::StdErrFormat);
}

void ValgrindProcess::handleRemoteStdout()
{
    const QString b = QString::fromUtf8(m_remote.m_process->readAllStandardOutput());
    if (!b.isEmpty())
        emit processOutput(b, Utils::StdOutFormat);
}

/// Remote
void ValgrindProcess::connected()
{
    QTC_ASSERT(m_remote.m_connection->state() == QSsh::SshConnection::Connected, return);

    emit localHostAddressRetrieved(m_remote.m_connection->connectionInfo().localAddress);

    // connected, run command
    QString cmd;

    if (!m_debuggee.workingDirectory.isEmpty())
        cmd += QString::fromLatin1("cd '%1' && ").arg(m_debuggee.workingDirectory);

    cmd += m_valgrindExecutable + QLatin1Char(' ') + argumentString(Utils::OsTypeLinux);

    m_remote.m_process = m_remote.m_connection->createRemoteProcess(cmd.toUtf8());
    for (auto it = m_debuggee.environment.constBegin(); it != m_debuggee.environment.constEnd(); ++it)
        m_remote.m_process->addToEnvironment(it.key().toUtf8(), it.value().toUtf8());

    connect(m_remote.m_process.data(), &QSsh::SshRemoteProcess::readyReadStandardError,
            this, &ValgrindProcess::handleRemoteStderr);
    connect(m_remote.m_process.data(), &QSsh::SshRemoteProcess::readyReadStandardOutput,
            this, &ValgrindProcess::handleRemoteStdout);
    connect(m_remote.m_process.data(), &QSsh::SshRemoteProcess::closed,
            this, &ValgrindProcess::closed);
    connect(m_remote.m_process.data(), &QSsh::SshRemoteProcess::started,
            this, &ValgrindProcess::remoteProcessStarted);
    m_remote.m_process->start();
}

QSsh::SshConnection *ValgrindProcess::connection() const
{
    return m_remote.m_connection;
}

bool ValgrindProcess::isLocal() const
{
    return m_device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}

void ValgrindProcess::localProcessStarted()
{
    m_pid = m_localProcess.applicationPID().pid();
    emit started();
}

void ValgrindProcess::remoteProcessStarted()
{
    QTC_ASSERT(m_remote.m_connection->state() == QSsh::SshConnection::Connected, return);

    // find out what PID our process has

    // NOTE: valgrind cloaks its name,
    // e.g.: valgrind --tool=memcheck foobar
    // => ps aux, pidof will see valgrind.bin
    // => pkill/killall/top... will see memcheck-amd64-linux or similar
    // hence we need to do something more complex...

    // plain path to exe, m_valgrindExe contains e.g. env vars etc. pp.
    const QString proc = m_valgrindExecutable.split(QLatin1Char(' ')).last();
    // sleep required since otherwise we might only match "bash -c..."
    //  and not the actual valgrind run
    const QString cmd = QString::fromLatin1("sleep 1; ps ax" // list all processes with aliased name
                                            " | grep '\\b%1.*%2'" // find valgrind process
                                            " | tail -n 1" // limit to single process
                                            // we pick the last one, first would be "bash -c ..."
                                            " | awk '{print $1;}'" // get pid
                                            ).arg(proc, Utils::FileName::fromString(m_debuggee.executable).fileName());

    m_remote.m_findPID = m_remote.m_connection->createRemoteProcess(cmd.toUtf8());
    connect(m_remote.m_findPID.data(), &QSsh::SshRemoteProcess::readyReadStandardError,
            this, &ValgrindProcess::handleRemoteStderr);
    connect(m_remote.m_findPID.data(), &QSsh::SshRemoteProcess::readyReadStandardOutput,
            this, &ValgrindProcess::findPIDOutputReceived);
    m_remote.m_findPID->start();
}

void ValgrindProcess::findPIDOutputReceived()
{
    bool ok;
    m_pid = m_remote.m_findPID->readAllStandardOutput().trimmed().toLongLong(&ok);
    if (!ok) {
        m_pid = 0;
        m_remote.m_errorString = tr("Could not determine remote PID.");
        m_remote.m_error = QProcess::FailedToStart;
        emit ValgrindProcess::error(QProcess::FailedToStart);
        close();
    } else {
        emit started();
    }
}

QString ValgrindProcess::argumentString(Utils::OsType osType) const
{
    QString arguments = Utils::QtcProcess::joinArgs(m_valgrindArguments, osType);
    if (!m_debuggee.executable.isEmpty())
        Utils::QtcProcess::addArg(&arguments, m_debuggee.executable, osType);
    Utils::QtcProcess::addArgs(&arguments, m_debuggee.commandLineArguments);
    return arguments;
}

void ValgrindProcess::closed(int status)
{
    QTC_ASSERT(m_remote.m_process, return);

    m_remote.m_errorString = m_remote.m_process->errorString();
    if (status == QSsh::SshRemoteProcess::FailedToStart) {
        m_remote.m_error = QProcess::FailedToStart;
        emit ValgrindProcess::error(QProcess::FailedToStart);
    } else if (status == QSsh::SshRemoteProcess::NormalExit) {
        emit finished(m_remote.m_process->exitCode(), QProcess::NormalExit);
    } else if (status == QSsh::SshRemoteProcess::CrashExit) {
        m_remote.m_error = QProcess::Crashed;
        emit finished(m_remote.m_process->exitCode(), QProcess::CrashExit);
    }
}

} // namespace Valgrind
