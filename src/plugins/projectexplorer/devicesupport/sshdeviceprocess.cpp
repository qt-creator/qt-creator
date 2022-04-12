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

#include <coreplugin/icore.h>
#include <ssh/sshconnection.h>
#include <ssh/sshconnectionmanager.h>
#include <ssh/sshremoteprocess.h>
#include <utils/environment.h>
#include <utils/processinterface.h>
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
    QSharedPointer<const IDevice> m_device;
    QSsh::SshConnection *connection = nullptr;
    QSsh::SshRemoteProcessPtr remoteProcess;
    QString processName;
    QString displayName;
    QString errorMessage;
    QProcess::ExitStatus exitStatus = QProcess::NormalExit;
    DeviceProcessSignalOperation::Ptr killOperation;
    QTimer killTimer;
    enum State { Inactive, Connecting, Connected, ProcessRunning } state = Inactive;

    void setState(State newState);
    void doSignal(Signal signal);
};

SshDeviceProcess::SshDeviceProcess(const IDevice::ConstPtr &device, QObject *parent)
    : QtcProcess(parent),
      d(std::make_unique<SshDeviceProcessPrivate>(this))
{
    d->m_device = device;
    connect(&d->killTimer, &QTimer::timeout, this, &SshDeviceProcess::handleKillOperationTimeout);
}

void SshDeviceProcess::emitStarted()
{
    handleProcessStarted();
}

void SshDeviceProcess::emitFinished()
{
    handleProcessFinished(QtcProcess::errorString());
}

const QSharedPointer<const IDevice> &SshDeviceProcess::device() const
{
    return d->m_device;
}

SshDeviceProcess::~SshDeviceProcess()
{
    d->setState(SshDeviceProcessPrivate::Inactive);
}

void SshDeviceProcess::startImpl()
{
    QTC_ASSERT(d->state == SshDeviceProcessPrivate::Inactive, return);
    QTC_ASSERT(usesTerminal() || !commandLine().isEmpty(), return);
    d->setState(SshDeviceProcessPrivate::Connecting);

    d->errorMessage.clear();
    d->exitStatus = QProcess::NormalExit;
    d->processName = commandLine().executable().toString();
    d->displayName = extraData("Ssh.X11ForwardToDisplay").toString();

    QSsh::SshConnectionParameters params = d->m_device->sshParameters();
    params.x11DisplayName = d->displayName;
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

ProcessResultData SshDeviceProcess::resultData() const
{
    const ProcessResultData result = QtcProcess::resultData();
    return { usesTerminal() ? result.m_exitCode : d->remoteProcess->exitCode(),
             d->exitStatus == QProcess::NormalExit && result.m_exitCode != 255
                            ? QProcess::NormalExit : QProcess::CrashExit,
             result.m_error,
             d->errorMessage };
}

QByteArray SshDeviceProcess::readAllStandardOutput()
{
    return d->remoteProcess.get() ? d->remoteProcess->readAllStandardOutput() : QByteArray();
}

QByteArray SshDeviceProcess::readAllStandardError()
{
    return d->remoteProcess.get() ? d->remoteProcess->readAllStandardError() : QByteArray();
}

void SshDeviceProcess::handleConnected()
{
    QTC_ASSERT(d->state == SshDeviceProcessPrivate::Connecting, return);
    d->setState(SshDeviceProcessPrivate::Connected);

    d->remoteProcess = usesTerminal() && d->processName.isEmpty()
            ? d->connection->createRemoteShell()
            : d->connection->createRemoteProcess(fullCommandLine());
    const QString display = d->displayName;
    if (!display.isEmpty())
        d->remoteProcess->requestX11Forwarding(display);
    if (usesTerminal()) {
        setAbortOnMetaChars(false);
        setCommand(d->remoteProcess->fullLocalCommandLine(true));
        QtcProcess::startImpl();
    } else {
        connect(d->remoteProcess.get(), &QtcProcess::started,
                this, &SshDeviceProcess::handleProcessStarted);
        connect(d->remoteProcess.get(), &QtcProcess::done, this, [this] {
                handleProcessFinished(d->remoteProcess->errorString());
                emit done();
        });
        connect(d->remoteProcess.get(), &QtcProcess::readyReadStandardOutput,
                this, &QtcProcess::readyReadStandardOutput);
        connect(d->remoteProcess.get(), &QtcProcess::readyReadStandardError,
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
        emit done();
        break;
    case SshDeviceProcessPrivate::ProcessRunning:
        d->exitStatus = QProcess::CrashExit;
        emit finished();
        emit done();
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

void SshDeviceProcess::handleProcessFinished(const QString &error)
{
    d->errorMessage = error;
    if (d->killOperation && error.isEmpty())
        d->errorMessage = tr("The process was ended forcefully.");
    d->setState(SshDeviceProcessPrivate::Inactive);
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
    emit done();
}

void SshDeviceProcess::handleKillOperationTimeout()
{
    d->exitStatus = QProcess::CrashExit; // Not entirely true, but it will get the message across.
    d->errorMessage = tr("Timeout waiting for remote process to finish.");
    d->setState(SshDeviceProcessPrivate::Inactive);
    emit finished();
    emit done();
}

QString SshDeviceProcess::fullCommandLine() const
{
    return commandLine().toUserOutput();
}

void SshDeviceProcess::SshDeviceProcessPrivate::doSignal(Signal signal)
{
    if (processName.isEmpty())
        return;
    switch (state) {
    case SshDeviceProcessPrivate::Inactive:
        QTC_ASSERT(false, return);
        break;
    case SshDeviceProcessPrivate::Connecting:
        errorMessage = tr("Terminated by request.");
        setState(SshDeviceProcessPrivate::Inactive);
        emit q->errorOccurred(QProcess::FailedToStart);
        emit q->done();
        break;
    case SshDeviceProcessPrivate::Connected:
    case SshDeviceProcessPrivate::ProcessRunning:
        DeviceProcessSignalOperation::Ptr signalOperation = m_device->signalOperation();
        const qint64 processId = q->processId();
        if (signal == Signal::Interrupt) {
            if (processId != 0)
                signalOperation->interruptProcess(processId);
            else
                signalOperation->interruptProcess(processName);
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
                signalOperation->killProcess(processName);
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
        if (q->usesTerminal())
            QMetaObject::invokeMethod(q, &QtcProcess::stopProcess, Qt::QueuedConnection);
    }
    killTimer.stop();
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
    QTC_ASSERT(!usesTerminal(), return -1);
    return d->remoteProcess->write(data);
}

} // namespace ProjectExplorer
