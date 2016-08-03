/****************************************************************************
**
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "gdbserverproviderprocess.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/runnables.h>

#include <utils/environment.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

#include <QStringList>

using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

GdbServerProviderProcess::GdbServerProviderProcess(
        const QSharedPointer<const ProjectExplorer::IDevice> &device,
        QObject *parent)
    : ProjectExplorer::DeviceProcess(device, parent)
    , m_process(new Utils::QtcProcess(this))
{
    if (Utils::HostOsInfo::isWindowsHost())
        m_process->setUseCtrlCStub(true);

    connect(m_process, &QProcess::errorOccurred, this, &GdbServerProviderProcess::error);
    connect(m_process, static_cast<void (QProcess::*)(int)>(&QProcess::finished),
            this, &GdbServerProviderProcess::finished);

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &ProjectExplorer::DeviceProcess::readyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &ProjectExplorer::DeviceProcess::readyReadStandardError);
    connect(m_process, &QProcess::started,
            this, &ProjectExplorer::DeviceProcess::started);
}

void GdbServerProviderProcess::start(const ProjectExplorer::Runnable &runnable)
{
    QTC_ASSERT(runnable.is<StandardRunnable>(), return);
    QTC_ASSERT(m_process->state() == QProcess::NotRunning, return);
    auto r = runnable.as<StandardRunnable>();
    m_process->setCommand(r.executable, r.commandLineArguments);
    m_process->start();
}

void GdbServerProviderProcess::interrupt()
{
    device()->signalOperation()->interruptProcess(m_process->processId());
}

void GdbServerProviderProcess::terminate()
{
    m_process->terminate();
}

void GdbServerProviderProcess::kill()
{
    m_process->kill();
}

QProcess::ProcessState GdbServerProviderProcess::state() const
{
    return m_process->state();
}

QProcess::ExitStatus GdbServerProviderProcess::exitStatus() const
{
    return m_process->exitStatus();
}

int GdbServerProviderProcess::exitCode() const
{
    return m_process->exitCode();
}

QString GdbServerProviderProcess::errorString() const
{
    return m_process->errorString();
}

QByteArray GdbServerProviderProcess::readAllStandardOutput()
{
    return m_process->readAllStandardOutput();
}

QByteArray GdbServerProviderProcess::readAllStandardError()
{
    return m_process->readAllStandardError();
}

qint64 GdbServerProviderProcess::write(const QByteArray &data)
{
    return m_process->write(data);
}

} // namespace Internal
} // namespace BareMetal
