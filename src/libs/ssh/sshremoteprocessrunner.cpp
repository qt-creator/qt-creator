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

#include "sshremoteprocessrunner.h"

#include "sshconnectionmanager.h"

#include <utils/qtcassert.h>

/*!
    \class QSsh::SshRemoteProcessRunner

    \brief The SshRemoteProcessRunner class is a convenience class for
    running a remote process over an SSH connection.
*/

namespace QSsh {
namespace Internal {
namespace {
enum State { Inactive, Connecting, Connected, ProcessRunning };
} // anonymous namespace

class SshRemoteProcessRunnerPrivate
{
public:
    SshRemoteProcessRunnerPrivate() : m_state(Inactive) {}

    SshRemoteProcessPtr m_process;
    SshConnection *m_connection;
    bool m_runInTerminal;
    QByteArray m_command;
    QString m_lastConnectionErrorString;
    QProcess::ExitStatus m_exitStatus;
    QByteArray m_stdout;
    QByteArray m_stderr;
    int m_exitCode;
    QString m_processErrorString;
    State m_state;
};

} // namespace Internal

using namespace Internal;

SshRemoteProcessRunner::SshRemoteProcessRunner(QObject *parent)
    : QObject(parent), d(new SshRemoteProcessRunnerPrivate)
{
}

SshRemoteProcessRunner::~SshRemoteProcessRunner()
{
    disconnect();
    setState(Inactive);
    delete d;
}

void SshRemoteProcessRunner::run(const QByteArray &command,
    const SshConnectionParameters &sshParams)
{
    QTC_ASSERT(d->m_state == Inactive, return);

    d->m_runInTerminal = false;
    runInternal(command, sshParams);
}

void SshRemoteProcessRunner::runInTerminal(const QByteArray &command,
                                           const SshConnectionParameters &sshParams)
{
    d->m_runInTerminal = true;
    runInternal(command, sshParams);
}

void SshRemoteProcessRunner::runInternal(const QByteArray &command,
    const SshConnectionParameters &sshParams)
{
    setState(Connecting);

    d->m_lastConnectionErrorString.clear();
    d->m_processErrorString.clear();
    d->m_exitCode = -1;
    d->m_command = command;
    d->m_connection = QSsh::acquireConnection(sshParams);
    connect(d->m_connection, &SshConnection::errorOccurred,
            this, &SshRemoteProcessRunner::handleConnectionError);
    connect(d->m_connection, &SshConnection::disconnected,
            this, &SshRemoteProcessRunner::handleDisconnected);
    if (d->m_connection->state() == SshConnection::Connected) {
        handleConnected();
    } else {
        connect(d->m_connection, &SshConnection::connected, this, &SshRemoteProcessRunner::handleConnected);
        if (d->m_connection->state() == SshConnection::Unconnected)
            d->m_connection->connectToHost();
    }
}

void SshRemoteProcessRunner::handleConnected()
{
    QTC_ASSERT(d->m_state == Connecting, return);
    setState(Connected);

    d->m_process = d->m_connection->createRemoteProcess(d->m_command);
    connect(d->m_process.get(), &SshRemoteProcess::started,
            this, &SshRemoteProcessRunner::handleProcessStarted);
    connect(d->m_process.get(), &SshRemoteProcess::done,
            this, &SshRemoteProcessRunner::handleProcessFinished);
    connect(d->m_process.get(), &SshRemoteProcess::readyReadStandardOutput,
            this, &SshRemoteProcessRunner::handleStdout);
    connect(d->m_process.get(), &SshRemoteProcess::readyReadStandardError,
            this, &SshRemoteProcessRunner::handleStderr);
    if (d->m_runInTerminal)
        d->m_process->requestTerminal();
    d->m_process->start();
}

void SshRemoteProcessRunner::handleConnectionError()
{
    d->m_lastConnectionErrorString = d->m_connection->errorString();
    handleDisconnected();
    emit connectionError();
}

void SshRemoteProcessRunner::handleDisconnected()
{
    QTC_ASSERT(d->m_state == Connecting || d->m_state == Connected || d->m_state == ProcessRunning,
               return);
    setState(Inactive);
}

void SshRemoteProcessRunner::handleProcessStarted()
{
    QTC_ASSERT(d->m_state == Connected, return);

    setState(ProcessRunning);
    emit processStarted();
}

void SshRemoteProcessRunner::handleProcessFinished(const QString &error)
{
    d->m_exitStatus = d->m_process->exitStatus();
    d->m_exitCode = d->m_process->exitCode();
    d->m_processErrorString = error;
    setState(Inactive);
    emit processClosed(d->m_processErrorString);
}

void SshRemoteProcessRunner::handleStdout()
{
    d->m_stdout += d->m_process->readAllStandardOutput();
    emit readyReadStandardOutput();
}

void SshRemoteProcessRunner::handleStderr()
{
    d->m_stderr += d->m_process->readAllStandardError();
    emit readyReadStandardError();
}

void SshRemoteProcessRunner::setState(int newState)
{
    if (d->m_state == newState)
        return;

    d->m_state = static_cast<State>(newState);
    if (d->m_state == Inactive) {
        if (d->m_process) {
            disconnect(d->m_process.get(), nullptr, this, nullptr);
            d->m_process->terminate();
            d->m_process.release()->deleteLater();
        }
        if (d->m_connection) {
            disconnect(d->m_connection, 0, this, 0);
            QSsh::releaseConnection(d->m_connection);
            d->m_connection = 0;
        }
    }
}

QByteArray SshRemoteProcessRunner::command() const { return d->m_command; }
QString SshRemoteProcessRunner::lastConnectionErrorString() const {
    return d->m_lastConnectionErrorString;
}

bool SshRemoteProcessRunner::isProcessRunning() const
{
    return d->m_process && d->m_process->isRunning();
}

SshRemoteProcess::ExitStatus SshRemoteProcessRunner::processExitStatus() const
{
    QTC_CHECK(!isProcessRunning());
    return d->m_exitStatus;
}

int SshRemoteProcessRunner::processExitCode() const
{
    QTC_CHECK(processExitStatus() == SshRemoteProcess::NormalExit);
    return d->m_exitCode;
}

QString SshRemoteProcessRunner::processErrorString() const
{
    return d->m_processErrorString;
}

QByteArray SshRemoteProcessRunner::readAllStandardOutput()
{
    const QByteArray data = d->m_stdout;
    d->m_stdout.clear();
    return data;
}

QByteArray SshRemoteProcessRunner::readAllStandardError()
{
    const QByteArray data = d->m_stderr;
    d->m_stderr.clear();
    return data;
}

void SshRemoteProcessRunner::writeDataToProcess(const QByteArray &data)
{
    QTC_CHECK(isProcessRunning());
    d->m_process->write(data);
}

void SshRemoteProcessRunner::cancel()
{
    setState(Inactive);
}

} // namespace QSsh
