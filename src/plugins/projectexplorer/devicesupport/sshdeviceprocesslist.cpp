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

#include <utils/processinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace QSsh;
using namespace Utils;

namespace ProjectExplorer {

class SshDeviceProcessListPrivate
{
public:
    QtcProcess m_process;
    DeviceProcessSignalOperation::Ptr m_signalOperation;
};

SshDeviceProcessList::SshDeviceProcessList(const IDevice::ConstPtr &device, QObject *parent) :
        DeviceProcessList(device, parent), d(std::make_unique<SshDeviceProcessListPrivate>())
{
    connect(&d->m_process, &QtcProcess::done, this, &SshDeviceProcessList::handleProcessDone);
}

SshDeviceProcessList::~SshDeviceProcessList() = default;

void SshDeviceProcessList::doUpdate()
{
    d->m_process.close();
    d->m_process.setCommand({device()->filePath("/bin/sh"), {"-c", listProcessesCommandLine()}});
    d->m_process.start();
}

void SshDeviceProcessList::doKillProcess(const ProcessInfo &process)
{
    d->m_signalOperation = device()->signalOperation();
    QTC_ASSERT(d->m_signalOperation, return);
    connect(d->m_signalOperation.data(), &DeviceProcessSignalOperation::finished,
            this, &SshDeviceProcessList::handleKillProcessFinished);
    d->m_signalOperation->killProcess(process.processId);
}

void SshDeviceProcessList::handleProcessDone()
{
    if (d->m_process.result() == ProcessResult::FinishedWithSuccess) {
        reportProcessListUpdated(buildProcessList(d->m_process.stdOut()));
    } else {
        const QString errorMessage = d->m_process.exitStatus() == QProcess::NormalExit
                ? tr("Process listing command failed with exit code %1.").arg(d->m_process.exitCode())
                : d->m_process.errorString();
        const QString stdErr = d->m_process.stdErr();
        const QString fullMessage = stdErr.isEmpty()
                ? errorMessage : errorMessage + '\n' + tr("Remote stderr was: %1").arg(stdErr);
        reportError(fullMessage);
    }
    setFinished();
}

void SshDeviceProcessList::handleKillProcessFinished(const QString &errorString)
{
    if (errorString.isEmpty())
        reportProcessKilled();
    else
        reportError(tr("Error: Kill process failed: %1").arg(errorString));
    setFinished();
}

void SshDeviceProcessList::setFinished()
{
    d->m_process.close();
    if (d->m_signalOperation) {
        d->m_signalOperation->disconnect(this);
        d->m_signalOperation.clear();
    }
}

} // namespace ProjectExplorer
