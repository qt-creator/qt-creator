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
#include "../runcontrol.h"

#include <coreplugin/icore.h>
#include <ssh/sshconnection.h>
#include <ssh/sshconnectionmanager.h>
#include <ssh/sshremoteprocess.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QString>
#include <QTimer>

using namespace Utils;

namespace ProjectExplorer {

enum class Signal { Interrupt, Terminate, Kill };

class SshDeviceProcess::SshDeviceProcessPrivate
{
public:
    SshDeviceProcessPrivate(SshDeviceProcess *q) : q(q) {}

    SshDeviceProcess * const q;
    bool ignoreFinished = true;
    QSsh::SshConnection *connection = nullptr;
    QSsh::SshRemoteProcessPtr remoteProcess;
    Runnable runnable;
    QString errorMessage;
    QProcess::ExitStatus exitStatus = QProcess::NormalExit;
    DeviceProcessSignalOperation::Ptr killOperation;
    QTimer killTimer;
    enum State { Inactive, Connecting, Connected, ProcessRunning } state = Inactive;

    void setState(State newState);
    void doSignal(Signal signal);

    QString displayName() const
    {
        return runnable.extraData.value("Ssh.X11ForwardToDisplay").toString();
    }
};

SshDeviceProcess::SshDeviceProcess(const IDevice::ConstPtr &device, QObject *parent)
    : DeviceProcess(device, QtcProcess::TerminalOn, parent),
      d(std::make_unique<SshDeviceProcessPrivate>(this))
{
    connect(this, &QtcProcess::finished, this, &SshDeviceProcess::handleThisProcessFinished);
    connect(&d->killTimer, &QTimer::timeout, this, &SshDeviceProcess::handleKillOperationTimeout);
}

SshDeviceProcess::~SshDeviceProcess()
{
    d->setState(SshDeviceProcessPrivate::Inactive);
}

void SshDeviceProcess::start(const Runnable &runnable)
{
    QTC_ASSERT(d->state == SshDeviceProcessPrivate::Inactive, return);
    QTC_ASSERT(runInTerminal() || !runnable.command.isEmpty(), return);
    d->setState(SshDeviceProcessPrivate::Connecting);

    d->errorMessage.clear();
    d->exitStatus = QProcess::NormalExit;
    d->runnable = runnable;
    QSsh::SshConnectionParameters params = device()->sshParameters();
    params.x11DisplayName = d->displayName();
    d->connection = QSsh::SshConnectionManager::acquireConnection(params);
    connect(d->connection, &QSsh::SshConnection::errorOccurred,
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
    d->doSignal(Signal::Interrupt);
}

void SshDeviceProcess::terminate()
{
    QTC_ASSERT(d->state == SshDeviceProcessPrivate::ProcessRunning, return);
    d->doSignal(Signal::Terminate);
}

void SshDeviceProcess::kill()
{
    QTC_ASSERT(d->state == SshDeviceProcessPrivate::ProcessRunning, return);
    d->doSignal(Signal::Kill);
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
    return d->exitStatus == QProcess::NormalExit && exitCode() != 255
            ? QProcess::NormalExit : QProcess::CrashExit;
}

int SshDeviceProcess::exitCode() const
{
    return runInTerminal() ? QtcProcess::exitCode() : d->remoteProcess->exitCode();
}

QString SshDeviceProcess::errorString() const
{
    return d->errorMessage;
}

QByteArray SshDeviceProcess::readAllStandardOutput()
{
    return d->remoteProcess.get() ? d->remoteProcess->readAllStandardOutput() : QByteArray();
}

QByteArray SshDeviceProcess::readAllStandardError()
{
    return d->remoteProcess.get() ? d->remoteProcess->readAllStandardError() : QByteArray();
}

qint64 SshDeviceProcess::processId() const
{
    return 0;
}

void SshDeviceProcess::handleConnected()
{
    QTC_ASSERT(d->state == SshDeviceProcessPrivate::Connecting, return);
    d->setState(SshDeviceProcessPrivate::Connected);

    d->remoteProcess = runInTerminal() && d->runnable.command.isEmpty()
            ? d->connection->createRemoteShell()
            : d->connection->createRemoteProcess(fullCommandLine(d->runnable));
    const QString display = d->displayName();
    if (!display.isEmpty())
        d->remoteProcess->requestX11Forwarding(display);
    if (runInTerminal()) {
        d->ignoreFinished = false;
        setAbortOnMetaChars(false);
        setCommand(d->remoteProcess->fullLocalCommandLine(true));
        QtcProcess::start();
    } else {
        connect(d->remoteProcess.get(), &QSsh::SshRemoteProcess::started,
                this, &SshDeviceProcess::handleProcessStarted);
        connect(d->remoteProcess.get(), &QSsh::SshRemoteProcess::done,
                this, &SshDeviceProcess::handleRemoteProcessFinished);
        connect(d->remoteProcess.get(), &QSsh::SshRemoteProcess::readyReadStandardOutput,
                this, &QtcProcess::readyReadStandardOutput);
        connect(d->remoteProcess.get(), &QSsh::SshRemoteProcess::readyReadStandardError,
                this, &QtcProcess::readyReadStandardError);
        d->remoteProcess->start();
    }
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
        emit errorOccurred(QProcess::FailedToStart);
        break;
    case SshDeviceProcessPrivate::ProcessRunning:
        d->exitStatus = QProcess::CrashExit;
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

void SshDeviceProcess::handleThisProcessFinished()
{
    if (d->ignoreFinished)
        return;
    // Hack: we rely on fact that this slot was called before any other external slot connected
    // to finished() signal. That's why we don't emit finished() signal from inside
    // handleProcessFinished() since this signal will reach all other external slots anyway.
    handleProcessFinished(QtcProcess::errorString(), false);
}

void SshDeviceProcess::handleRemoteProcessFinished(const QString &error)
{
    handleProcessFinished(error, true);
}

void SshDeviceProcess::handleProcessFinished(const QString &error, bool emitFinished)
{
    d->errorMessage = error;
    if (d->killOperation && error.isEmpty())
        d->errorMessage = tr("The process was ended forcefully.");
    d->setState(SshDeviceProcessPrivate::Inactive);
    if (emitFinished)
        emit finished();
}

void SshDeviceProcess::handleKillOperationFinished(const QString &errorMessage)
{
    QTC_ASSERT(d->state == SshDeviceProcessPrivate::ProcessRunning, return);
    if (errorMessage.isEmpty()) // Process will finish as expected; nothing to do here.
        return;

    d->exitStatus = QProcess::CrashExit; // Not entirely true, but it will get the message across.
    d->errorMessage = tr("Failed to kill remote process: %1").arg(errorMessage);
    d->setState(SshDeviceProcessPrivate::Inactive);
    emit finished();
}

void SshDeviceProcess::handleKillOperationTimeout()
{
    d->exitStatus = QProcess::CrashExit; // Not entirely true, but it will get the message across.
    d->errorMessage = tr("Timeout waiting for remote process to finish.");
    d->setState(SshDeviceProcessPrivate::Inactive);
    emit finished();
}

QString SshDeviceProcess::fullCommandLine(const Runnable &runnable) const
{
    QString cmdLine = runnable.command.executable().toString();
    // FIXME: That quotes wrongly.
    if (!runnable.command.arguments().isEmpty())
        cmdLine.append(QLatin1Char(' ')).append(runnable.command.arguments());
    return cmdLine;
}

void SshDeviceProcess::SshDeviceProcessPrivate::doSignal(Signal signal)
{
    if (runnable.command.isEmpty())
        return;
    switch (state) {
    case SshDeviceProcessPrivate::Inactive:
        QTC_ASSERT(false, return);
        break;
    case SshDeviceProcessPrivate::Connecting:
        errorMessage = tr("Terminated by request.");
        setState(SshDeviceProcessPrivate::Inactive);
        emit q->errorOccurred(QProcess::FailedToStart);
        break;
    case SshDeviceProcessPrivate::Connected:
    case SshDeviceProcessPrivate::ProcessRunning:
        DeviceProcessSignalOperation::Ptr signalOperation = q->device()->signalOperation();
        const qint64 processId = q->processId();
        if (signal == Signal::Interrupt) {
            if (processId != 0)
                signalOperation->interruptProcess(processId);
            else
                signalOperation->interruptProcess(runnable.command.executable().toString());
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
                signalOperation->killProcess(runnable.command.executable().toString());
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
        if (q->runInTerminal())
            QMetaObject::invokeMethod(q, &QtcProcess::stopProcess, Qt::QueuedConnection);
    }
    killTimer.stop();
    ignoreFinished = true;
    if (remoteProcess)
        remoteProcess->disconnect(q);
    if (connection) {
        connection->disconnect(q);
        QSsh::SshConnectionManager::releaseConnection(connection);
        connection = nullptr;
    }
}

qint64 SshDeviceProcess::write(const QByteArray &data)
{
    QTC_ASSERT(!runInTerminal(), return -1);
    return d->remoteProcess->write(data);
}

} // namespace ProjectExplorer
