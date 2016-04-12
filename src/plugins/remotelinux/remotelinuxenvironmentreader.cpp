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

#include "remotelinuxenvironmentreader.h"

#include <projectexplorer/devicesupport/deviceprocess.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/runnables.h>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

RemoteLinuxEnvironmentReader::RemoteLinuxEnvironmentReader(const IDevice::ConstPtr &device,
                                                           QObject *parent)
    : QObject(parent)
    , m_stop(false)
    , m_env(Utils::OsTypeLinux)
    , m_device(device)
    , m_deviceProcess(0)
{
}

void RemoteLinuxEnvironmentReader::start()
{
    if (!m_device) {
        emit error(tr("Error: No device"));
        setFinished();
        return;
    }
    m_stop = false;
    m_deviceProcess = m_device->createProcess(this);
    connect(m_deviceProcess, &DeviceProcess::error,
            this, &RemoteLinuxEnvironmentReader::handleError);
    connect(m_deviceProcess, &DeviceProcess::finished,
            this, &RemoteLinuxEnvironmentReader::remoteProcessFinished);
    StandardRunnable runnable;
    runnable.executable = QLatin1String("env");
    m_deviceProcess->start(runnable);
}

void RemoteLinuxEnvironmentReader::stop()
{
    m_stop = true;
    destroyProcess();
}

void RemoteLinuxEnvironmentReader::handleError()
{
    if (m_stop)
        return;

    emit error(tr("Error: %1").arg(m_deviceProcess->errorString()));
    setFinished();
}

void RemoteLinuxEnvironmentReader::handleCurrentDeviceConfigChanged()
{
    m_env.clear();
    setFinished();
}

void RemoteLinuxEnvironmentReader::remoteProcessFinished()
{
    if (m_stop)
        return;

    m_env.clear();
    QString errorMessage;
    if (m_deviceProcess->exitStatus() != QProcess::NormalExit) {
        errorMessage = m_deviceProcess->errorString();
    } else if (m_deviceProcess->exitCode() != 0) {
        errorMessage = tr("Process exited with code %1.")
                .arg(m_deviceProcess->exitCode());
    }
    if (!errorMessage.isEmpty()) {
        errorMessage = tr("Error running 'env': %1").arg(errorMessage);
        const QString remoteStderr
                = QString::fromUtf8(m_deviceProcess->readAllStandardError()).trimmed();
        if (!remoteStderr.isEmpty())
            errorMessage += QLatin1Char('\n') + tr("Remote stderr was: \"%1\"").arg(remoteStderr);
        emit error(errorMessage);
    } else {
        QString remoteOutput = QString::fromUtf8(m_deviceProcess->readAllStandardOutput());
        if (!remoteOutput.isEmpty()) {
            m_env = Utils::Environment(remoteOutput.split(QLatin1Char('\n'),
                QString::SkipEmptyParts), Utils::OsTypeLinux);
        }
    }
    setFinished();
}

void RemoteLinuxEnvironmentReader::setFinished()
{
    stop();
    emit finished();
}

void RemoteLinuxEnvironmentReader::destroyProcess()
{
    if (!m_deviceProcess)
        return;
    m_deviceProcess->disconnect(this);
    if (m_deviceProcess->state() != QProcess::NotRunning)
        m_deviceProcess->terminate();
    m_deviceProcess->deleteLater();
    m_deviceProcess = 0;
}

} // namespace Internal
} // namespace RemoteLinux
