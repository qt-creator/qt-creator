/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "sshdeviceprocess.h"

#include "idevice.h"

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
    bool serverSupportsSignals;
    QSsh::SshConnection *connection;
    QSsh::SshRemoteProcess::Ptr process;
    QString executable;
    QStringList arguments;
    QString errorMessage;
    QSsh::SshRemoteProcess::ExitStatus exitStatus;
    DeviceProcessSignalOperation::Ptr killOperation;
    QTimer killTimer;
    QByteArray stdOut;
    QByteArray stdErr;
    int exitCode;
    enum State { Inactive, Connecting, Connected, ProcessRunning } state;
    Utils::Environment environment;

    void setState(State newState);
    void doSignal(QSsh::SshRemoteProcess::Signal signal);
};

SshDeviceProcess::SshDeviceProcess(const IDevice::ConstPtr &device, QObject *parent)
    : DeviceProcess(device, parent), d(new SshDeviceProcessPrivate(this))
{
    d->connection = 0;
    d->state = SshDeviceProcessPrivate::Inactive;
    setSshServerSupportsSignals(false);
    connect(&d->killTimer, SIGNAL(timeout()), SLOT(handleKillOperationTimeout()));
}

SshDeviceProcess::~SshDeviceProcess()
{
    d->setState(SshDeviceProcessPrivate::Inactive);
    delete d;
}

void SshDeviceProcess::start(const QString &executable, const QStringList &arguments)
{
    QTC_ASSERT(d->state == SshDeviceProcessPrivate::Inactive, return);
    d->setState(SshDeviceProcessPrivate::Connecting);

    d->errorMessage.clear();
    d->exitCode = -1;
    d->executable = executable;
    d->arguments = arguments;
    d->connection = QSsh::acquireConnection(device()->sshParameters());
    connect(d->connection, SIGNAL(error(QSsh::SshError)), SLOT(handleConnectionError()));
    connect(d->connection, SIGNAL(disconnected()), SLOT(handleDisconnected()));
    if (d->connection->state() == QSsh::SshConnection::Connected) {
        handleConnected();
    } else {
        connect(d->connection, SIGNAL(connected()), SLOT(handleConnected()));
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

QString SshDeviceProcess::executable() const
{
    return d->executable;
}

QStringList SshDeviceProcess::arguments() const
{
    return d->arguments;
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

Utils::Environment SshDeviceProcess::environment() const
{
    return d->environment;
}

void SshDeviceProcess::setEnvironment(const Utils::Environment &env)
{
    d->environment = env;
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

void SshDeviceProcess::handleConnected()
{
    QTC_ASSERT(d->state == SshDeviceProcessPrivate::Connecting, return);
    d->setState(SshDeviceProcessPrivate::Connected);

    d->process = d->connection->createRemoteProcess(fullCommandLine().toUtf8());
    connect(d->process.data(), SIGNAL(started()), SLOT(handleProcessStarted()));
    connect(d->process.data(), SIGNAL(closed(int)), SLOT(handleProcessFinished(int)));
    connect(d->process.data(), SIGNAL(readyReadStandardOutput()), SLOT(handleStdout()));
    connect(d->process.data(), SIGNAL(readyReadStandardError()), SLOT(handleStderr()));

    d->process->clearEnvironment();
    const Utils::Environment env = environment();
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

QString SshDeviceProcess::fullCommandLine() const
{
    QString cmdLine = executable();
    if (!arguments().isEmpty())
        cmdLine.append(QLatin1Char(' ')).append(arguments().join(QLatin1Char(' ')));
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
            if (signal == QSsh::SshRemoteProcess::IntSignal) {
                signalOperation->interruptProcess(executable);
            } else {
                if (killOperation) // We are already in the process of killing the app.
                    return;
                killOperation = signalOperation;
                connect(signalOperation.data(), SIGNAL(finished(QString)), q,
                        SLOT(handleKillOperationFinished(QString)));
                killTimer.start(5000);
                signalOperation->killProcess(executable);
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
