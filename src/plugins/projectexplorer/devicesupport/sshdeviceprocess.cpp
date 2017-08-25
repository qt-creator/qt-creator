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

#include "sshdeviceprocess.h"

#include "idevice.h"
#include "../runnables.h"

#include <ssh/sshconnection.h>
#include <ssh/sshconnectionmanager.h>
#include <ssh/sshremoteprocess.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QString>
#include <QTimer>

namespace ProjectExplorer {

class SshDeviceProcess::SshDeviceProcessPrivate
{
public:
    SshDeviceProcessPrivate(SshDeviceProcess *q) : q(q) {}

    SshDeviceProcess * const q;
    bool serverSupportsSignals = false;
    QSsh::SshConnection *connection = nullptr;
    QSsh::SshRemoteProcess::Ptr process;
    StandardRunnable runnable;
    QString errorMessage;
    QSsh::SshRemoteProcess::ExitStatus exitStatus;
    DeviceProcessSignalOperation::Ptr killOperation;
    QTimer killTimer;
    QByteArray stdOut;
    QByteArray stdErr;
    int exitCode = -1;
    enum State { Inactive, Connecting, Connected, ProcessRunning } state = Inactive;

    void setState(State newState);
    void doSignal(QSsh::SshRemoteProcess::Signal signal);
};

SshDeviceProcess::SshDeviceProcess(const IDevice::ConstPtr &device, QObject *parent)
    : DeviceProcess(device, parent), d(new SshDeviceProcessPrivate(this))
{
    connect(&d->killTimer, &QTimer::timeout, this, &SshDeviceProcess::handleKillOperationTimeout);
}

SshDeviceProcess::~SshDeviceProcess()
{
    d->setState(SshDeviceProcessPrivate::Inactive);
    delete d;
}

void SshDeviceProcess::start(const Runnable &runnable)
{
    QTC_ASSERT(d->state == SshDeviceProcessPrivate::Inactive, return);
    if (!runnable.is<StandardRunnable>()) {
        d->errorMessage = tr("Internal error");
        error(QProcess::FailedToStart);
        return;
    }
    d->setState(SshDeviceProcessPrivate::Connecting);

    d->errorMessage.clear();
    d->exitCode = -1;
    d->runnable = runnable.as<StandardRunnable>();
    d->connection = QSsh::acquireConnection(device()->sshParameters());
    connect(d->connection, &QSsh::SshConnection::error,
            this, &SshDeviceProcess::handleConnectionError);
    connect(d->connection, &QSsh::SshConnection::disconnected,
            this, &SshDeviceProcess::handleDisconnected);
    if (d->connection->state() == QSsh::SshConnection::Connected) {
        handleConnected();
    } else {
        connect(d->connection, &QSsh::SshConnection::connected,
                this, &SshDeviceProcess::handleConnected);
        if (d->connection->state() == QSsh::SshConnection::Unconnected)
            d->connection->connectToHost();
    }
}

void SshDeviceProcess::interrupt()
{
    QTC_ASSERT(d->state == SshDeviceProcessPrivate::ProcessRunning, return);
    d->doSignal(QSsh::SshRemoteProcess::IntSignal);
}

void SshDeviceProcess::terminate()
{
    d->doSignal(QSsh::SshRemoteProcess::TermSignal);
}

void SshDeviceProcess::kill()
{
    d->doSignal(QSsh::SshRemoteProcess::KillSignal);
}

QProcess::ProcessState SshDeviceProcess::state() const
{
    switch (d->state) {
    case SshDeviceProcessPrivate::Inactive:
        return QProcess::NotRunning;
    case SshDeviceProcessPrivate::Connecting:
    case SshDeviceProcessPrivate::Connected:
        return QProcess::Starting;
    case SshDeviceProcessPrivate::ProcessRunning:
        return QProcess::Running;
    default:
        QTC_CHECK(false);
        return QProcess::NotRunning;
    }
}

QProcess::ExitStatus SshDeviceProcess::exitStatus() const
{
    return d->exitStatus == QSsh::SshRemoteProcess::NormalExit
            ? QProcess::NormalExit : QProcess::CrashExit;
}

int SshDeviceProcess::exitCode() const
{
    return d->exitCode;
}

QString SshDeviceProcess::errorString() const
{
    return d->errorMessage;
}

QByteArray SshDeviceProcess::readAllStandardOutput()
{
    const QByteArray data = d->stdOut;
    d->stdOut.clear();
    return data;
}

QByteArray SshDeviceProcess::readAllStandardError()
{
    const QByteArray data = d->stdErr;
    d->stdErr.clear();
    return data;
}

void SshDeviceProcess::setSshServerSupportsSignals(bool signalsSupported)
{
    d->serverSupportsSignals = signalsSupported;
}

qint64 SshDeviceProcess::processId() const
{
    return 0;
}

void SshDeviceProcess::handleConnected()
{
    QTC_ASSERT(d->state == SshDeviceProcessPrivate::Connecting, return);
    d->setState(SshDeviceProcessPrivate::Connected);

    d->process = d->connection->createRemoteProcess(fullCommandLine(d->runnable).toUtf8());
    connect(d->process.data(), &QSsh::SshRemoteProcess::started, this, &SshDeviceProcess::handleProcessStarted);
    connect(d->process.data(), &QSsh::SshRemoteProcess::closed, this, &SshDeviceProcess::handleProcessFinished);
    connect(d->process.data(), &QSsh::SshRemoteProcess::readyReadStandardOutput, this, &SshDeviceProcess::handleStdout);
    connect(d->process.data(), &QSsh::SshRemoteProcess::readyReadStandardError, this, &SshDeviceProcess::handleStderr);

    d->process->clearEnvironment();
    const Utils::Environment env = d->runnable.environment;
    for (Utils::Environment::const_iterator it = env.constBegin(); it != env.constEnd(); ++it)
        d->process->addToEnvironment(env.key(it).toUtf8(), env.value(it).toUtf8());
    d->process->start();
}

void SshDeviceProcess::handleConnectionError()
{
    QTC_ASSERT(d->state != SshDeviceProcessPrivate::Inactive, return);

    d->errorMessage = d->connection->errorString();
    handleDisconnected();
}

void SshDeviceProcess::handleDisconnected()
{
    QTC_ASSERT(d->state != SshDeviceProcessPrivate::Inactive, return);
    const SshDeviceProcessPrivate::State oldState = d->state;
    d->setState(SshDeviceProcessPrivate::Inactive);
    switch (oldState) {
    case SshDeviceProcessPrivate::Connecting:
    case SshDeviceProcessPrivate::Connected:
        emit error(QProcess::FailedToStart);
        break;
    case SshDeviceProcessPrivate::ProcessRunning:
        d->exitStatus = QSsh::SshRemoteProcess::CrashExit;
        emit finished();
    default:
        break;
    }
}

void SshDeviceProcess::handleProcessStarted()
{
    QTC_ASSERT(d->state == SshDeviceProcessPrivate::Connected, return);

    d->setState(SshDeviceProcessPrivate::ProcessRunning);
    emit started();
}

void SshDeviceProcess::handleProcessFinished(int exitStatus)
{
    d->exitStatus = static_cast<QSsh::SshRemoteProcess::ExitStatus>(exitStatus);
    switch (d->exitStatus) {
    case QSsh::SshRemoteProcess::FailedToStart:
        QTC_ASSERT(d->state == SshDeviceProcessPrivate::Connected, return);
        break;
    case QSsh::SshRemoteProcess::CrashExit:
        QTC_ASSERT(d->state == SshDeviceProcessPrivate::ProcessRunning, return);
        break;
    case QSsh::SshRemoteProcess::NormalExit:
        QTC_ASSERT(d->state == SshDeviceProcessPrivate::ProcessRunning, return);
        d->exitCode = d->process->exitCode();
        break;
    default:
        QTC_ASSERT(false, return);
    }
    d->errorMessage = d->process->errorString();
    d->setState(SshDeviceProcessPrivate::Inactive);
    emit finished();
}

void SshDeviceProcess::handleStdout()
{
    d->stdOut += d->process->readAllStandardOutput();
    emit readyReadStandardOutput();
}

void SshDeviceProcess::handleStderr()
{
    d->stdErr += d->process->readAllStandardError();
    emit readyReadStandardError();
}

void SshDeviceProcess::handleKillOperationFinished(const QString &errorMessage)
{
    QTC_ASSERT(d->state == SshDeviceProcessPrivate::ProcessRunning, return);
    if (errorMessage.isEmpty()) // Process will finish as expected; nothing to do here.
        return;

    d->exitStatus = QSsh::SshRemoteProcess::CrashExit; // Not entirely true, but it will get the message across.
    d->errorMessage = tr("Failed to kill remote process: %1").arg(errorMessage);
    d->setState(SshDeviceProcessPrivate::Inactive);
    emit finished();
}

void SshDeviceProcess::handleKillOperationTimeout()
{
    d->exitStatus = QSsh::SshRemoteProcess::CrashExit; // Not entirely true, but it will get the message across.
    d->errorMessage = tr("Timeout waiting for remote process to finish.");
    d->setState(SshDeviceProcessPrivate::Inactive);
    emit finished();
}

QString SshDeviceProcess::fullCommandLine(const StandardRunnable &runnable) const
{
    QString cmdLine = runnable.executable;
    if (!runnable.commandLineArguments.isEmpty())
        cmdLine.append(QLatin1Char(' ')).append(runnable.commandLineArguments);
    return cmdLine;
}

void SshDeviceProcess::SshDeviceProcessPrivate::doSignal(QSsh::SshRemoteProcess::Signal signal)
{
    switch (state) {
    case SshDeviceProcessPrivate::Inactive:
        QTC_ASSERT(false, return);
        break;
    case SshDeviceProcessPrivate::Connecting:
        errorMessage = tr("Terminated by request.");
        setState(SshDeviceProcessPrivate::Inactive);
        emit q->error(QProcess::FailedToStart);
        break;
    case SshDeviceProcessPrivate::Connected:
    case SshDeviceProcessPrivate::ProcessRunning:
        if (serverSupportsSignals) {
            process->sendSignal(signal);
        } else {
            DeviceProcessSignalOperation::Ptr signalOperation = q->device()->signalOperation();
            quint64 processId = q->processId();
            if (signal == QSsh::SshRemoteProcess::IntSignal) {
                if (processId != 0)
                    signalOperation->interruptProcess(processId);
                else
                    signalOperation->interruptProcess(runnable.executable);
            } else {
                if (killOperation) // We are already in the process of killing the app.
                    return;
                killOperation = signalOperation;
                connect(signalOperation.data(), &DeviceProcessSignalOperation::finished, q,
                        &SshDeviceProcess::handleKillOperationFinished);
                killTimer.start(5000);
                if (processId != 0)
                    signalOperation->killProcess(processId);
                else
                    signalOperation->killProcess(runnable.executable);
            }
        }
        break;
    }
}

void SshDeviceProcess::SshDeviceProcessPrivate::setState(SshDeviceProcess::SshDeviceProcessPrivate::State newState)
{
    if (state == newState)
        return;

    state = newState;
    if (state != Inactive)
        return;

    if (killOperation) {
        killOperation->disconnect(q);
        killOperation.clear();
    }
    killTimer.stop();
    if (process)
        process->disconnect(q);
    if (connection) {
        connection->disconnect(q);
        QSsh::releaseConnection(connection);
        connection = 0;
    }
}

qint64 SshDeviceProcess::write(const QByteArray &data)
{
    return d->process->write(data);
}

} // namespace ProjectExplorer
