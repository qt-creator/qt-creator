// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sshdeviceprocesslist.h"

#include "idevice.h"
#include "../projectexplorertr.h"

#include <utils/processinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

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
        reportProcessListUpdated(buildProcessList(d->m_process.cleanedStdOut()));
    } else {
        const QString errorString = d->m_process.exitStatus() == QProcess::NormalExit
                ? Tr::tr("Process listing command failed with exit code %1.").arg(d->m_process.exitCode())
                : d->m_process.errorString();
        const QString stdErr = d->m_process.cleanedStdErr();
        const QString outputString
            = stdErr.isEmpty() ? stdErr : Tr::tr("Remote stderr was: %1").arg(stdErr);
        reportError(Utils::joinStrings({errorString, outputString}, '\n'));
    }
    setFinished();
}

void SshDeviceProcessList::handleKillProcessFinished(const QString &errorString)
{
    if (errorString.isEmpty())
        reportProcessKilled();
    else
        reportError(Tr::tr("Error: Kill process failed: %1").arg(errorString));
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
