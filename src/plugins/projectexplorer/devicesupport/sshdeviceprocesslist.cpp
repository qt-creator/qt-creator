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

#include "sshdeviceprocesslist.h"

#include "idevice.h"

#include <ssh/sshremoteprocessrunner.h>
#include <utils/qtcassert.h>

using namespace QSsh;

namespace ProjectExplorer {

class SshDeviceProcessList::SshDeviceProcessListPrivate
{
public:
    SshRemoteProcessRunner process;
    DeviceProcessSignalOperation::Ptr signalOperation;
};

SshDeviceProcessList::SshDeviceProcessList(const IDevice::ConstPtr &device, QObject *parent) :
        DeviceProcessList(device, parent), d(new SshDeviceProcessListPrivate)
{
}

SshDeviceProcessList::~SshDeviceProcessList()
{
    delete d;
}

void SshDeviceProcessList::doUpdate()
{
    connect(&d->process, &SshRemoteProcessRunner::connectionError,
            this, &SshDeviceProcessList::handleConnectionError);
    connect(&d->process, &SshRemoteProcessRunner::processClosed,
            this, &SshDeviceProcessList::handleListProcessFinished);
    d->process.run(listProcessesCommandLine().toUtf8(), device()->sshParameters());
}

void SshDeviceProcessList::doKillProcess(const DeviceProcessItem &process)
{
    d->signalOperation = device()->signalOperation();
    QTC_ASSERT(d->signalOperation, return);
    connect(d->signalOperation.data(), &DeviceProcessSignalOperation::finished,
            this, &SshDeviceProcessList::handleKillProcessFinished);
    d->signalOperation->killProcess(process.pid);
}

void SshDeviceProcessList::handleConnectionError()
{
    setFinished();
    reportError(tr("Connection failure: %1").arg(d->process.lastConnectionErrorString()));
}

void SshDeviceProcessList::handleListProcessFinished(int exitStatus)
{
    setFinished();
    switch (exitStatus) {
    case SshRemoteProcess::FailedToStart:
        handleProcessError(tr("Error: Process listing command failed to start: %1")
                .arg(d->process.processErrorString()));
        break;
    case SshRemoteProcess::CrashExit:
        handleProcessError(tr("Error: Process listing command crashed: %1")
                .arg(d->process.processErrorString()));
        break;
    case SshRemoteProcess::NormalExit:
        if (d->process.processExitCode() == 0) {
            const QByteArray remoteStdout = d->process.readAllStandardOutput();
            const QString stdoutString
                    = QString::fromUtf8(remoteStdout.data(), remoteStdout.count());
            reportProcessListUpdated(buildProcessList(stdoutString));
        } else {
            handleProcessError(tr("Process listing command failed with exit code %1.")
                    .arg(d->process.processExitCode()));
        }
        break;
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid exit status");
    }
}

void SshDeviceProcessList::handleKillProcessFinished(const QString &errorString)
{
    if (errorString.isEmpty())
        reportProcessKilled();
    else
        reportError(tr("Error: Kill process failed: %1").arg(errorString));
    setFinished();
}

void SshDeviceProcessList::handleProcessError(const QString &errorMessage)
{
    QString fullMessage = errorMessage;
    const QByteArray remoteStderr = d->process.readAllStandardError();
    if (!remoteStderr.isEmpty())
        fullMessage += QLatin1Char('\n') + tr("Remote stderr was: %1").arg(QString::fromUtf8(remoteStderr));
    reportError(fullMessage);
}

void SshDeviceProcessList::setFinished()
{
    d->process.disconnect(this);
    if (d->signalOperation) {
        d->signalOperation->disconnect(this);
        d->signalOperation.clear();
    }
}

} // namespace ProjectExplorer
