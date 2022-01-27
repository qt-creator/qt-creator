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

#include "desktopdeviceprocess.h"

#include "idevice.h"
#include "../runcontrol.h"

#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

DesktopDeviceProcess::DesktopDeviceProcess(const QSharedPointer<const IDevice> &device,
                                           QObject *parent)
    : DeviceProcess(device, ProcessMode::Writer, parent)
{
    connect(process(), &QtcProcess::errorOccurred, this, &DeviceProcess::errorOccurred);
    connect(process(), &QtcProcess::finished, this, &DeviceProcess::finished);
    connect(process(), &QtcProcess::readyReadStandardOutput,
            this, &DeviceProcess::readyReadStandardOutput);
    connect(process(), &QtcProcess::readyReadStandardError,
            this, &DeviceProcess::readyReadStandardError);
    connect(process(), &QtcProcess::started, this, &DeviceProcess::started);
}

void DesktopDeviceProcess::start(const Runnable &runnable)
{
    QTC_ASSERT(process()->state() == QProcess::NotRunning, return);
    if (runnable.environment.size())
        process()->setEnvironment(runnable.environment);
    process()->setWorkingDirectory(runnable.workingDirectory);
    process()->setCommand(runnable.command);
    process()->start();
}

void DesktopDeviceProcess::interrupt()
{
    device()->signalOperation()->interruptProcess(process()->processId());
}

void DesktopDeviceProcess::terminate()
{
    process()->terminate();
}

void DesktopDeviceProcess::kill()
{
    process()->kill();
}

QProcess::ProcessState DesktopDeviceProcess::state() const
{
    return process()->state();
}

QProcess::ExitStatus DesktopDeviceProcess::exitStatus() const
{
    return process()->exitStatus();
}

int DesktopDeviceProcess::exitCode() const
{
    return process()->exitCode();
}

QString DesktopDeviceProcess::errorString() const
{
    return process()->errorString();
}

QByteArray DesktopDeviceProcess::readAllStandardOutput()
{
    return process()->readAllStandardOutput();
}

QByteArray DesktopDeviceProcess::readAllStandardError()
{
    return process()->readAllStandardError();
}

qint64 DesktopDeviceProcess::write(const QByteArray &data)
{
    return process()->write(data);
}

} // namespace Internal
} // namespace ProjectExplorer
