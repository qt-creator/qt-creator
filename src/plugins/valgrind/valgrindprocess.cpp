/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "valgrindprocess.h"

#include <QDebug>
#include <QEventLoop>
#include <QFileInfo>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

namespace Valgrind {

ValgrindProcess::ValgrindProcess(bool isLocal, const QSsh::SshConnectionParameters &sshParams,
                                 QSsh::SshConnection *connection, QObject *parent)
    : QObject(parent),
      m_isLocal(isLocal),
      m_localRunMode(ProjectExplorer::ApplicationLauncher::Gui)
{
    m_remote.m_params = sshParams;
    m_remote.m_connection = connection;
    m_remote.m_error = QProcess::UnknownError;
    m_pid = 0;
}

void ValgrindProcess::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    if (isLocal())
        m_localProcess.setProcessChannelMode(mode);
    ///TODO: remote support this by handling the mode internally
}

void ValgrindProcess::setWorkingDirectory(const QString &path)
{
    if (isLocal())
        m_localProcess.setWorkingDirectory(path);
    else
        m_remote.m_workingDir = path;
}

QString ValgrindProcess::workingDirectory() const
{
    if (isLocal())
        return m_localProcess.workingDirectory();
    else
        return m_remote.m_workingDir;
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

void ValgrindProcess::setValgrindArguments(const QStringList &valgrindArguments)
{
    m_valgrindArguments = valgrindArguments;
}

void ValgrindProcess::setDebuggeeExecutable(const QString &debuggeeExecutable)
{
    m_debuggeeExecutable = debuggeeExecutable;
}

void ValgrindProcess::setDebugeeArguments(const QString &debuggeeArguments)
{
    m_debuggeeArguments = debuggeeArguments;
}

void ValgrindProcess::setEnvironment(const Utils::Environment &environment)
{
    if (isLocal())
        m_localProcess.setEnvironment(environment);
    ///TODO: remote anything that should/could be done here?
}

void ValgrindProcess::setLocalRunMode(ProjectExplorer::ApplicationLauncher::Mode localRunMode)
{
    m_localRunMode = localRunMode;
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

void ValgrindProcess::run()
{
    if (isLocal()) {
        connect(&m_localProcess, SIGNAL(processExited(int,QProcess::ExitStatus)),
                this, SIGNAL(finished(int,QProcess::ExitStatus)));
        connect(&m_localProcess, SIGNAL(processStarted()),
                this, SLOT(localProcessStarted()));
        connect(&m_localProcess, SIGNAL(error(QProcess::ProcessError)),
                this, SIGNAL(error(QProcess::ProcessError)));
        connect(&m_localProcess, SIGNAL(appendMessage(QString,Utils::OutputFormat)),
                this, SIGNAL(processOutput(QString,Utils::OutputFormat)));

        m_localProcess.start(m_localRunMode, m_valgrindExecutable,
                             argumentString(Utils::HostOsInfo::hostOs()));

    } else {
        m_remote.m_valgrindExe = m_valgrindExecutable;
        m_remote.m_debuggee = m_debuggeeExecutable;

        // connect to host and wait for connection
        if (!m_remote.m_connection)
            m_remote.m_connection = new QSsh::SshConnection(m_remote.m_params, this);

        if (m_remote.m_connection->state() != QSsh::SshConnection::Connected) {
            connect(m_remote.m_connection, SIGNAL(connected()), this, SLOT(connected()));
            connect(m_remote.m_connection, SIGNAL(error(QSsh::SshError)),
                    this, SLOT(handleError(QSsh::SshError)));
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

QProcess::ProcessError ValgrindProcess::error() const
{
    if (isLocal())
        return m_localProcess.error();
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

    if (!m_remote.m_workingDir.isEmpty())
        cmd += QString::fromLatin1("cd '%1' && ").arg(m_remote.m_workingDir);

    cmd += m_remote.m_valgrindExe + QLatin1Char(' ') + argumentString(Utils::OsTypeLinux);

    m_remote.m_process = m_remote.m_connection->createRemoteProcess(cmd.toUtf8());
    connect(m_remote.m_process.data(), SIGNAL(readyReadStandardError()),
            this, SLOT(handleRemoteStderr()));
    connect(m_remote.m_process.data(), SIGNAL(readyReadStandardOutput()),
            this, SLOT(handleRemoteStdout()));
    connect(m_remote.m_process.data(), SIGNAL(closed(int)),
            this, SLOT(closed(int)));
    connect(m_remote.m_process.data(), SIGNAL(started()),
            this, SLOT(remoteProcessStarted()));
    m_remote.m_process->start();
}

QSsh::SshConnection *ValgrindProcess::connection() const
{
    return m_remote.m_connection;
}

void ValgrindProcess::localProcessStarted()
{
    m_pid = m_localProcess.applicationPID();
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
    const QString proc = m_remote.m_valgrindExe.split(QLatin1Char(' ')).last();
    // sleep required since otherwise we might only match "bash -c..."
    //  and not the actual valgrind run
    const QString cmd = QString::fromLatin1("sleep 1; ps ax" // list all processes with aliased name
                                            " | grep '\\b%1.*%2'" // find valgrind process
                                            " | tail -n 1" // limit to single process
                                            // we pick the last one, first would be "bash -c ..."
                                            " | awk '{print $1;}'" // get pid
                                            ).arg(proc, QFileInfo(m_remote.m_debuggee).fileName());

    m_remote.m_findPID = m_remote.m_connection->createRemoteProcess(cmd.toUtf8());
    connect(m_remote.m_findPID.data(), SIGNAL(readyReadStandardError()),
            this, SLOT(handleRemoteStderr()));
    connect(m_remote.m_findPID.data(), SIGNAL(readyReadStandardOutput()),
            this, SLOT(findPIDOutputReceived()));
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
    Utils::QtcProcess::addArg(&arguments, m_debuggeeExecutable, osType);
    Utils::QtcProcess::addArgs(&arguments, m_debuggeeArguments);
    return arguments;
}


///////////

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
