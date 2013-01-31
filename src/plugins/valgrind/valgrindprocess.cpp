/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifdef Q_OS_WIN
#    include <qt_windows.h>
#endif

namespace Valgrind {

ValgrindProcess::ValgrindProcess(QObject *parent)
    : QObject(parent)
{
}

////////////////////////

LocalValgrindProcess::LocalValgrindProcess(QObject *parent)
: ValgrindProcess(parent)
{
    connect(&m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SIGNAL(finished(int,QProcess::ExitStatus)));
    connect(&m_process, SIGNAL(started()),
            this, SIGNAL(started()));
    connect(&m_process, SIGNAL(error(QProcess::ProcessError)),
            this, SIGNAL(error(QProcess::ProcessError)));
    connect(&m_process, SIGNAL(readyReadStandardError()),
            this, SLOT(readyReadStandardError()));
    connect(&m_process, SIGNAL(readyReadStandardOutput()),
            this, SLOT(readyReadStandardOutput()));
}

void LocalValgrindProcess::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    m_process.setProcessChannelMode(mode);
}

void LocalValgrindProcess::setWorkingDirectory(const QString &path)
{
    m_process.setWorkingDirectory(path);
}

QString LocalValgrindProcess::workingDirectory() const
{
    return m_process.workingDirectory();
}

bool LocalValgrindProcess::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

void LocalValgrindProcess::setEnvironment(const Utils::Environment &environment)
{
    m_process.setEnvironment(environment);
}

void LocalValgrindProcess::close()
{
    m_process.terminate();
}

void LocalValgrindProcess::run(const QString &valgrindExecutable, const QStringList &valgrindArguments,
                                const QString &debuggeeExecutable, const QString &debuggeeArguments)
{
    QString arguments;
    Utils::QtcProcess::addArgs(&arguments, valgrindArguments);

    Utils::QtcProcess::addArg(&arguments, debuggeeExecutable);
    Utils::QtcProcess::addArgs(&arguments, debuggeeArguments);

    m_process.setCommand(valgrindExecutable, arguments);
    m_process.start();
    m_process.waitForStarted();
#ifdef Q_OS_WIN
    m_pid = m_process.pid()->dwProcessId;
#else
    m_pid = m_process.pid();
#endif
}

QString LocalValgrindProcess::errorString() const
{
    return m_process.errorString();
}

QProcess::ProcessError LocalValgrindProcess::error() const
{
    return m_process.error();
}

qint64 LocalValgrindProcess::pid() const
{
    return m_pid;
}

void LocalValgrindProcess::readyReadStandardError()
{
    const QByteArray b = m_process.readAllStandardError();
    if (!b.isEmpty())
        emit processOutput(b, Utils::StdErrFormat);
}

void LocalValgrindProcess::readyReadStandardOutput()
{
    const QByteArray b = m_process.readAllStandardOutput();
    if (!b.isEmpty())
        emit processOutput(b, Utils::StdOutFormat);
}

////////////////////////

RemoteValgrindProcess::RemoteValgrindProcess(const QSsh::SshConnectionParameters &sshParams,
                                             QObject *parent)
    : ValgrindProcess(parent)
    , m_params(sshParams)
    , m_connection(0)
    , m_error(QProcess::UnknownError)
    , m_pid(0)
{}

RemoteValgrindProcess::RemoteValgrindProcess(QSsh::SshConnection *connection, QObject *parent)
    : ValgrindProcess(parent)
    , m_params(connection->connectionParameters())
    , m_connection(connection)
    , m_error(QProcess::UnknownError)
    , m_pid(0)
{}

RemoteValgrindProcess::~RemoteValgrindProcess()
{
}

bool RemoteValgrindProcess::isRunning() const
{
    return m_process && m_process->isRunning();
}

void RemoteValgrindProcess::run(const QString &valgrindExecutable, const QStringList &valgrindArguments,
                                const QString &debuggeeExecutable, const QString &debuggeeArguments)
{
    m_valgrindExe = valgrindExecutable;
    m_debuggee = debuggeeExecutable;
    m_debuggeeArgs = debuggeeArguments;
    m_valgrindArgs = valgrindArguments;

    // connect to host and wait for connection
    if (!m_connection)
        m_connection = new QSsh::SshConnection(m_params, this);

    if (m_connection->state() != QSsh::SshConnection::Connected) {
        connect(m_connection, SIGNAL(connected()), this, SLOT(connected()));
        connect(m_connection, SIGNAL(error(QSsh::SshError)),
                this, SLOT(error(QSsh::SshError)));
        if (m_connection->state() == QSsh::SshConnection::Unconnected)
            m_connection->connectToHost();
    } else {
        connected();
    }
}

void RemoteValgrindProcess::connected()
{
    QTC_ASSERT(m_connection->state() == QSsh::SshConnection::Connected, return);

    // connected, run command
    QString cmd;

    if (!m_workingDir.isEmpty())
        cmd += QString::fromLatin1("cd '%1' && ").arg(m_workingDir);

    QString arguments;
    Utils::QtcProcess::addArgs(&arguments, m_valgrindArgs);
    Utils::QtcProcess::addArg(&arguments, m_debuggee);
    Utils::QtcProcess::addArgs(&arguments, m_debuggeeArgs);
    cmd += m_valgrindExe + QLatin1Char(' ') + arguments;

    m_process = m_connection->createRemoteProcess(cmd.toUtf8());
    connect(m_process.data(), SIGNAL(readyReadStandardError()), this, SLOT(standardError()));
    connect(m_process.data(), SIGNAL(readyReadStandardOutput()), this, SLOT(standardOutput()));
    connect(m_process.data(), SIGNAL(closed(int)),
            this, SLOT(closed(int)));
    connect(m_process.data(), SIGNAL(started()),
            this, SLOT(processStarted()));
    m_process->start();
}

QSsh::SshConnection *RemoteValgrindProcess::connection() const
{
    return m_connection;
}

void RemoteValgrindProcess::processStarted()
{
    QTC_ASSERT(m_connection->state() == QSsh::SshConnection::Connected, return);

    // find out what PID our process has

    // NOTE: valgrind cloaks its name,
    // e.g.: valgrind --tool=memcheck foobar
    // => ps aux, pidof will see valgrind.bin
    // => pkill/killall/top... will see memcheck-amd64-linux or similar
    // hence we need to do something more complex...

    // plain path to exe, m_valgrindExe contains e.g. env vars etc. pp.
    const QString proc = m_valgrindExe.split(QLatin1Char(' ')).last();
    // sleep required since otherwise we might only match "bash -c..."
    //  and not the actual valgrind run
    const QString cmd = QString::fromLatin1("sleep 1; ps ax" // list all processes with aliased name
                                            " | grep '\\b%1.*%2'" // find valgrind process
                                            " | tail -n 1" // limit to single process
                                            // we pick the last one, first would be "bash -c ..."
                                            " | awk '{print $1;}'" // get pid
                                            ).arg(proc, QFileInfo(m_debuggee).fileName());

    m_findPID = m_connection->createRemoteProcess(cmd.toUtf8());
    connect(m_findPID.data(), SIGNAL(readyReadStandardError()), this, SLOT(standardError()));
    connect(m_findPID.data(), SIGNAL(readyReadStandardOutput()), SLOT(findPIDOutputReceived()));
    m_findPID->start();
}

void RemoteValgrindProcess::findPIDOutputReceived()
{
    bool ok;
    m_pid = m_findPID->readAllStandardOutput().trimmed().toLongLong(&ok);
    if (!ok) {
        m_pid = 0;
        m_errorString = tr("Could not determine remote PID.");
        m_error = QProcess::FailedToStart;
        emit ValgrindProcess::error(QProcess::FailedToStart);
        close();
    } else {
        emit started();
    }
}

void RemoteValgrindProcess::standardOutput()
{
    emit processOutput(m_process->readAllStandardOutput(), Utils::StdOutFormat);
}

void RemoteValgrindProcess::standardError()
{
    emit processOutput(m_process->readAllStandardError(), Utils::StdErrFormat);
}

void RemoteValgrindProcess::error(QSsh::SshError error)
{
    switch (error) {
        case QSsh::SshTimeoutError:
            m_error = QProcess::Timedout;
            break;
        default:
            m_error = QProcess::FailedToStart;
            break;
    }
    m_errorString = m_connection->errorString();
    emit ValgrindProcess::error(m_error);
}

void RemoteValgrindProcess::close()
{
    QTC_ASSERT(m_connection->state() == QSsh::SshConnection::Connected, return);
    if (m_process) {
        if (m_pid) {
            const QString killTemplate = QString::fromLatin1("kill -%2 %1" // kill
                                                ).arg(m_pid);

            const QString niceKill = killTemplate.arg(QLatin1String("SIGTERM"));
            const QString brutalKill = killTemplate.arg(QLatin1String("SIGKILL"));
            const QString remoteCall = niceKill + QLatin1String("; sleep 1; ") + brutalKill;

            QSsh::SshRemoteProcess::Ptr cleanup = m_connection->createRemoteProcess(remoteCall.toUtf8());
            cleanup->start();
        }
    }
}

void RemoteValgrindProcess::closed(int status)
{
    QTC_ASSERT(m_process, return);

    m_errorString = m_process->errorString();
    if (status == QSsh::SshRemoteProcess::FailedToStart) {
        m_error = QProcess::FailedToStart;
        emit ValgrindProcess::error(QProcess::FailedToStart);
    } else if (status == QSsh::SshRemoteProcess::NormalExit) {
        emit finished(m_process->exitCode(), QProcess::NormalExit);
    } else if (status == QSsh::SshRemoteProcess::CrashExit) {
        m_error = QProcess::Crashed;
        emit finished(m_process->exitCode(), QProcess::CrashExit);
    }
}

QString RemoteValgrindProcess::errorString() const
{
    return m_errorString;
}

QProcess::ProcessError RemoteValgrindProcess::error() const
{
    return m_error;
}

void RemoteValgrindProcess::setEnvironment(const Utils::Environment &environment)
{
    Q_UNUSED(environment);
    ///TODO: anything that should/could be done here?
}

void RemoteValgrindProcess::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    Q_UNUSED(mode);
    ///TODO: support this by handling the mode internally
}

void RemoteValgrindProcess::setWorkingDirectory(const QString &path)
{
    m_workingDir = path;
}

QString RemoteValgrindProcess::workingDirectory() const
{
    return m_workingDir;
}

qint64 RemoteValgrindProcess::pid() const
{
    return m_pid;
}

} // namespace Valgrind
