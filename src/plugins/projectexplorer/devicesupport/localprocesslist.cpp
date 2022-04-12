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
