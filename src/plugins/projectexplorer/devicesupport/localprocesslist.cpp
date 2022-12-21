// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "localprocesslist.h"

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
namespace Internal {

LocalProcessList::LocalProcessList(const IDevice::ConstPtr &device, QObject *parent)
    : DeviceProcessList(device, parent)
{
#if defined(Q_OS_UNIX)
    setOwnPid(getpid());
#elif defined(Q_OS_WIN)
    setOwnPid(GetCurrentProcessId());
#endif
}

void LocalProcessList::doKillProcess(const ProcessInfo &processInfo)
{
    DeviceProcessSignalOperation::Ptr signalOperation = device()->signalOperation();
    connect(signalOperation.data(), &DeviceProcessSignalOperation::finished,
            this, &LocalProcessList::reportDelayedKillStatus);
    signalOperation->killProcess(processInfo.processId);
}

void LocalProcessList::handleUpdate()
{
    reportProcessListUpdated(ProcessInfo::processInfoList());
}

void LocalProcessList::doUpdate()
{
    QTimer::singleShot(0, this, &LocalProcessList::handleUpdate);
}

void LocalProcessList::reportDelayedKillStatus(const QString &errorMessage)
{
    if (errorMessage.isEmpty())
        reportProcessKilled();
    else
        reportError(errorMessage);
}

} // namespace Internal
} // namespace ProjectExplorer
