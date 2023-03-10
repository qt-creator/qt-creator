// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processlist.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <utils/processinfo.h>

#include <QTimer>

#if defined(Q_OS_UNIX)
#include <unistd.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif

using namespace Utils;

namespace ProjectExplorer {

ProcessList::ProcessList(const IDevice::ConstPtr &device, QObject *parent)
    : DeviceProcessList(device, parent)
{
#if defined(Q_OS_UNIX)
    setOwnPid(getpid());
#elif defined(Q_OS_WIN)
    setOwnPid(GetCurrentProcessId());
#endif
}

void ProcessList::doKillProcess(const ProcessInfo &processInfo)
{
    m_signalOperation = device()->signalOperation();
    connect(m_signalOperation.data(),
            &DeviceProcessSignalOperation::finished,
            this,
            &ProcessList::reportDelayedKillStatus);
    m_signalOperation->killProcess(processInfo.processId);
}

void ProcessList::handleUpdate()
{
    reportProcessListUpdated(ProcessInfo::processInfoList(DeviceProcessList::device()->rootPath()));
}

void ProcessList::doUpdate()
{
    QTimer::singleShot(0, this, &ProcessList::handleUpdate);
}

void ProcessList::reportDelayedKillStatus(const QString &errorMessage)
{
    if (errorMessage.isEmpty())
        reportProcessKilled();
    else
        reportError(errorMessage);

    m_signalOperation.reset();
}

} // namespace ProjectExplorer
