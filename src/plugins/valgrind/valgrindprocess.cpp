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

ValgrindProcess::ValgrindProcess(const IDevice::ConstPtr &device, QObject *parent)
    : QObject(parent), m_device(device)
{
}

ValgrindProcess::~ValgrindProcess()
{
}

void ValgrindProcess::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    m_valgrindProcess.setProcessChannelMode(mode);
}

QString ValgrindProcess::workingDirectory() const
{
    return m_debuggee.workingDirectory;
}

bool ValgrindProcess::isRunning() const
{
    return m_valgrindProcess.isRunning();
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
    m_valgrindProcess.stop();
}

void ValgrindProcess::run(ApplicationLauncher::Mode runMode)
{
    connect(&m_valgrindProcess, &ApplicationLauncher::processExited,
            this, &ValgrindProcess::finished);
    connect(&m_valgrindProcess, &ApplicationLauncher::processStarted,
            this, &ValgrindProcess::localProcessStarted);
    connect(&m_valgrindProcess, &ApplicationLauncher::error,
            this, &ValgrindProcess::error);
    connect(&m_valgrindProcess, &ApplicationLauncher::appendMessage,
            this, &ValgrindProcess::processOutput);

    connect(&m_valgrindProcess, &ApplicationLauncher::remoteStderr,
            this, &ValgrindProcess::handleRemoteStderr);
    connect(&m_valgrindProcess, &ApplicationLauncher::remoteStdout,
            this, &ValgrindProcess::handleRemoteStdout);
    connect(&m_valgrindProcess, &ApplicationLauncher::finished,
            this, &ValgrindProcess::closed);
    connect(&m_valgrindProcess, &ApplicationLauncher::remoteProcessStarted,
            this, &ValgrindProcess::remoteProcessStarted);

    StandardRunnable valgrind;
    valgrind.executable = m_valgrindExecutable;
    valgrind.workingDirectory = m_debuggee.workingDirectory;
    valgrind.environment = m_debuggee.environment;
    valgrind.runMode = runMode;
    valgrind.device = m_device;

    if (isLocal()) {
        valgrind.commandLineArguments = argumentString(Utils::HostOsInfo::hostOs());
        m_valgrindProcess.start(valgrind);
    } else {
        valgrind.commandLineArguments = argumentString(Utils::OsTypeLinux);
        m_valgrindProcess.start(valgrind, m_device);
    }
}

QString ValgrindProcess::errorString() const
{
    return m_valgrindProcess.errorString();
}

QProcess::ProcessError ValgrindProcess::processError() const
{
    return m_valgrindProcess.processError();
}

qint64 ValgrindProcess::pid() const
{
    return m_pid;
}

void ValgrindProcess::handleRemoteStderr(const QByteArray &b)
{
    if (!b.isEmpty())
        emit processOutput(QString::fromUtf8(b), Utils::StdErrFormat);
}

void ValgrindProcess::handleRemoteStdout(const QByteArray &b)
{
    if (!b.isEmpty())
        emit processOutput(QString::fromUtf8(b), Utils::StdOutFormat);
}

bool ValgrindProcess::isLocal() const
{
    return m_device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}

void ValgrindProcess::localProcessStarted()
{
    m_pid = m_valgrindProcess.applicationPID().pid();
    emit started();
}

void ValgrindProcess::remoteProcessStarted()
{
    // find out what PID our process has

    // NOTE: valgrind cloaks its name,
    // e.g.: valgrind --tool=memcheck foobar
    // => ps aux, pidof will see valgrind.bin
    // => pkill/killall/top... will see memcheck-amd64-linux or similar
    // hence we need to do something more complex...

    // plain path to exe, m_valgrindExe contains e.g. env vars etc. pp.
    const QString proc = m_valgrindExecutable.split(' ').last();

    StandardRunnable findPid;
    findPid.executable = "/bin/sh";
    // sleep required since otherwise we might only match "bash -c..."
    //  and not the actual valgrind run
    findPid.commandLineArguments = QString("-c \""
                                           "sleep 1; ps ax" // list all processes with aliased name
                                           " | grep '\\b%1.*%2'" // find valgrind process
                                           " | tail -n 1" // limit to single process
                                           // we pick the last one, first would be "bash -c ..."
                                           " | awk '{print $1;}'" // get pid
                                           "\""
                                           ).arg(proc, Utils::FileName::fromString(m_debuggee.executable).fileName());

//    m_remote.m_findPID = m_remote.m_connection->createRemoteProcess(cmd.toUtf8());
    connect(&m_findPID, &ApplicationLauncher::remoteStderr,
            this, &ValgrindProcess::handleRemoteStderr);
    connect(&m_findPID, &ApplicationLauncher::remoteStdout,
            this, &ValgrindProcess::findPIDOutputReceived);
    m_findPID.start(findPid, m_device);
}

void ValgrindProcess::findPIDOutputReceived(const QByteArray &out)
{
    if (out.isEmpty())
        return;
    bool ok;
    m_pid = out.trimmed().toLongLong(&ok);
    if (!ok) {
        m_pid = 0;
//        m_remote.m_errorString = tr("Could not determine remote PID.");
//        emit ValgrindProcess::error(QProcess::FailedToStart);
//        close();
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

void ValgrindProcess::closed(bool success)
{
    Q_UNUSED(success);
//    QTC_ASSERT(m_remote.m_process, return);

//    m_remote.m_errorString = m_remote.m_process->errorString();
//    if (status == QSsh::SshRemoteProcess::FailedToStart) {
//        m_remote.m_error = QProcess::FailedToStart;
//        emit ValgrindProcess::error(QProcess::FailedToStart);
//    } else if (status == QSsh::SshRemoteProcess::NormalExit) {
//        emit finished(m_remote.m_process->exitCode(), QProcess::NormalExit);
//    } else if (status == QSsh::SshRemoteProcess::CrashExit) {
//        m_remote.m_error = QProcess::Crashed;
//        emit finished(m_remote.m_process->exitCode(), QProcess::CrashExit);
//    }
     emit finished(0, QProcess::NormalExit);
}

} // namespace Valgrind
