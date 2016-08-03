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
#include "../runnables.h"

#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

namespace ProjectExplorer {
namespace Internal {

DesktopDeviceProcess::DesktopDeviceProcess(const QSharedPointer<const IDevice> &device,
                                           QObject *parent)
    : DeviceProcess(device, parent)
{
    connect(&m_process, &QProcess::errorOccurred, this, &DeviceProcess::error);
    connect(&m_process, static_cast<void (QProcess::*)(int)>(&QProcess::finished),
            this, &DeviceProcess::finished);
    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &DeviceProcess::readyReadStandardOutput);
    connect(&m_process, &QProcess::readyReadStandardError,
            this, &DeviceProcess::readyReadStandardError);
    connect(&m_process, &QProcess::started, this, &DeviceProcess::started);
}

void DesktopDeviceProcess::start(const Runnable &runnable)
{
    QTC_ASSERT(runnable.is<StandardRunnable>(), return);
    QTC_ASSERT(m_process.state() == QProcess::NotRunning, return);
    auto r = runnable.as<StandardRunnable>();
    m_process.setProcessEnvironment(r.environment.toProcessEnvironment());
    m_process.setWorkingDirectory(r.workingDirectory);
    m_process.start(r.executable, Utils::QtcProcess::splitArgs(r.commandLineArguments));
}

void DesktopDeviceProcess::interrupt()
{
    device()->signalOperation()->interruptProcess(m_process.processId());
}

void DesktopDeviceProcess::terminate()
{
    m_process.terminate();
}

void DesktopDeviceProcess::kill()
{
    m_process.kill();
}

QProcess::ProcessState DesktopDeviceProcess::state() const
{
    return m_process.state();
}

QProcess::ExitStatus DesktopDeviceProcess::exitStatus() const
{
    return m_process.exitStatus();
}

int DesktopDeviceProcess::exitCode() const
{
    return m_process.exitCode();
}

QString DesktopDeviceProcess::errorString() const
{
    return m_process.errorString();
}

QByteArray DesktopDeviceProcess::readAllStandardOutput()
{
    return m_process.readAllStandardOutput();
}

QByteArray DesktopDeviceProcess::readAllStandardError()
{
    return m_process.readAllStandardError();
}

qint64 DesktopDeviceProcess::write(const QByteArray &data)
{
    return m_process.write(data);
}

} // namespace Internal
} // namespace ProjectExplorer
