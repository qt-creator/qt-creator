// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "remotelinuxenvironmentreader.h"

#include "remotelinuxtr.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/runcontrol.h>

#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

RemoteLinuxEnvironmentReader::RemoteLinuxEnvironmentReader(const IDevice::ConstPtr &device,
                                                           QObject *parent)
    : QObject(parent)
    , m_env(Utils::OsTypeLinux)
    , m_device(device)
{
}

void RemoteLinuxEnvironmentReader::start()
{
    if (!m_device) {
        emit error(Tr::tr("Error: No device"));
        setFinished();
        return;
    }
    m_deviceProcess = new QtcProcess(this);
    connect(m_deviceProcess, &QtcProcess::done,
            this, &RemoteLinuxEnvironmentReader::handleDone);
    m_deviceProcess->setCommand({m_device->filePath("env"), {}});
    m_deviceProcess->start();
}

void RemoteLinuxEnvironmentReader::stop()
{
    if (!m_deviceProcess)
        return;
    m_deviceProcess->disconnect(this);
    m_deviceProcess->deleteLater();
    m_deviceProcess = nullptr;
}

void RemoteLinuxEnvironmentReader::handleCurrentDeviceConfigChanged()
{
    m_env.clear();
    setFinished();
}

void RemoteLinuxEnvironmentReader::handleDone()
{
    if (m_deviceProcess->result() != ProcessResult::FinishedWithSuccess) {
        emit error(Tr::tr("Error: %1").arg(m_deviceProcess->errorString()));
        setFinished();
        return;
    }

    m_env.clear();
    QString errorMessage;
    if (m_deviceProcess->exitStatus() != QProcess::NormalExit) {
        errorMessage = m_deviceProcess->errorString();
    } else if (m_deviceProcess->exitCode() != 0) {
        errorMessage = Tr::tr("Process exited with code %1.")
                .arg(m_deviceProcess->exitCode());
    }

    if (!errorMessage.isEmpty()) {
        errorMessage = Tr::tr("Error running 'env': %1").arg(errorMessage);
        const QString remoteStderr
                = QString::fromUtf8(m_deviceProcess->readAllStandardError()).trimmed();
        if (!remoteStderr.isEmpty())
            errorMessage += QLatin1Char('\n') + Tr::tr("Remote stderr was: \"%1\"").arg(remoteStderr);
        emit error(errorMessage);
    } else {
        const QString remoteOutput = QString::fromUtf8(m_deviceProcess->readAllStandardOutput());
        if (!remoteOutput.isEmpty()) {
            m_env = Utils::Environment(remoteOutput.split(QLatin1Char('\n'),
                Qt::SkipEmptyParts), Utils::OsTypeLinux);
        }
    }
    setFinished();
}

void RemoteLinuxEnvironmentReader::setFinished()
{
    stop();
    emit finished();
}

} // namespace Internal
} // namespace RemoteLinux
