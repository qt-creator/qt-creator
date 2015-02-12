/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "desktopdeviceprocess.h"
#include "idevice.h"

#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

namespace ProjectExplorer {
namespace Internal {

DesktopDeviceProcess::DesktopDeviceProcess(const QSharedPointer<const IDevice> &device,
                                           QObject *parent)
    : DeviceProcess(device, parent), m_process(new QProcess(this))
{
    connect(m_process, SIGNAL(error(QProcess::ProcessError)),
            SIGNAL(error(QProcess::ProcessError)));
    connect(m_process, SIGNAL(finished(int)), SIGNAL(finished()));
    connect(m_process, SIGNAL(readyReadStandardOutput()), SIGNAL(readyReadStandardOutput()));
    connect(m_process, SIGNAL(readyReadStandardError()), SIGNAL(readyReadStandardError()));
    connect(m_process, SIGNAL(started()), SIGNAL(started()));
}

void DesktopDeviceProcess::start(const QString &executable, const QStringList &arguments)
{
    QTC_ASSERT(m_process->state() == QProcess::NotRunning, return);
    m_process->start(executable, arguments);
}

void DesktopDeviceProcess::interrupt()
{
    device()->signalOperation()->interruptProcess(Utils::qPidToPid(m_process->pid()));
}

void DesktopDeviceProcess::terminate()
{
    m_process->terminate();
}

void DesktopDeviceProcess::kill()
{
    m_process->kill();
}

QProcess::ProcessState DesktopDeviceProcess::state() const
{
    return m_process->state();
}

QProcess::ExitStatus DesktopDeviceProcess::exitStatus() const
{
    return m_process->exitStatus();
}

int DesktopDeviceProcess::exitCode() const
{
    return m_process->exitCode();
}

QString DesktopDeviceProcess::errorString() const
{
    return m_process->errorString();
}

Utils::Environment DesktopDeviceProcess::environment() const
{
    return Utils::Environment(m_process->processEnvironment().toStringList());
}

void DesktopDeviceProcess::setEnvironment(const Utils::Environment &env)
{
    m_process->setProcessEnvironment(env.toProcessEnvironment());
}

void DesktopDeviceProcess::setWorkingDirectory(const QString &directory)
{
    m_process->setWorkingDirectory(directory);
}

QByteArray DesktopDeviceProcess::readAllStandardOutput()
{
    return m_process->readAllStandardOutput();
}

QByteArray DesktopDeviceProcess::readAllStandardError()
{
    return m_process->readAllStandardError();
}

qint64 DesktopDeviceProcess::write(const QByteArray &data)
{
    return m_process->write(data);
}

} // namespace Internal
} // namespace ProjectExplorer
