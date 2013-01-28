/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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
#include "sshdeviceprocesslist.h"

#include <ssh/sshremoteprocessrunner.h>
#include <utils/qtcassert.h>

using namespace QSsh;

namespace ProjectExplorer {

class SshDeviceProcessList::SshDeviceProcessListPrivate
{
public:
    SshRemoteProcessRunner process;
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
    QTC_ASSERT(device()->processSupport(), return);
    connect(&d->process, SIGNAL(connectionError()), SLOT(handleConnectionError()));
    connect(&d->process, SIGNAL(processClosed(int)), SLOT(handleListProcessFinished(int)));
    d->process.run(listProcessesCommandLine().toUtf8(), device()->sshParameters());
}

void SshDeviceProcessList::doKillProcess(const DeviceProcess &process)
{
    QTC_ASSERT(device()->processSupport(), return);
    connect(&d->process, SIGNAL(connectionError()), SLOT(handleConnectionError()));
    connect(&d->process, SIGNAL(processClosed(int)), SLOT(handleKillProcessFinished(int)));
    d->process.run(device()->processSupport()->killProcessByPidCommandLine(process.pid).toUtf8(),
                   device()->sshParameters());
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

void SshDeviceProcessList::handleKillProcessFinished(int exitStatus)
{
    setFinished();
    switch (exitStatus) {
    case SshRemoteProcess::FailedToStart:
        handleProcessError(tr("Error: Kill process failed to start: %1")
                .arg(d->process.processErrorString()));
        break;
    case SshRemoteProcess::CrashExit:
        handleProcessError(tr("Error: Kill process crashed: %1")
                .arg(d->process.processErrorString()));
        break;
    case SshRemoteProcess::NormalExit: {
        const int exitCode = d->process.processExitCode();
        if (exitCode == 0)
            reportProcessKilled();
        else
            handleProcessError(tr("Kill process failed with exit code %1.").arg(exitCode));
        break;
    }
    default:
        Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid exit status");
    }
}

void SshDeviceProcessList::handleProcessError(const QString &errorMessage)
{
    QString fullMessage = errorMessage;
    const QByteArray remoteStderr = d->process.readAllStandardError();
    if (!remoteStderr.isEmpty())
        fullMessage += tr("\nRemote stderr was: %1").arg(QString::fromUtf8(remoteStderr));
    reportError(fullMessage);
}

void SshDeviceProcessList::setFinished()
{
    d->process.disconnect(this);
}

} // namespace ProjectExplorer
