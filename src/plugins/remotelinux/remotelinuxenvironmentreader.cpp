/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "remotelinuxenvironmentreader.h"

#include "linuxdevice.h"

#include <ssh/sshremoteprocessrunner.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

RemoteLinuxEnvironmentReader::RemoteLinuxEnvironmentReader(RunConfiguration *config, QObject *parent)
    : QObject(parent)
    , m_stop(false)
    , m_kit(config->target()->kit())
    , m_remoteProcessRunner(0)
{
    connect(config->target(), SIGNAL(kitChanged()),
        this, SLOT(handleCurrentDeviceConfigChanged()));
}

void RemoteLinuxEnvironmentReader::start(const QString &environmentSetupCommand)
{
    IDevice::ConstPtr device = DeviceKitInformation::device(m_kit);
    if (!device)
        return;
    m_stop = false;
    if (!m_remoteProcessRunner)
        m_remoteProcessRunner = new QSsh::SshRemoteProcessRunner(this);
    connect(m_remoteProcessRunner, SIGNAL(connectionError()), SLOT(handleConnectionFailure()));
    connect(m_remoteProcessRunner, SIGNAL(processClosed(int)), SLOT(remoteProcessFinished(int)));
    const QByteArray remoteCall
        = QString(environmentSetupCommand + QLatin1String("; env")).toUtf8();
    m_remoteProcessRunner->run(remoteCall, device->sshParameters());
}

void RemoteLinuxEnvironmentReader::stop()
{
    m_stop = true;
    if (m_remoteProcessRunner)
        disconnect(m_remoteProcessRunner, 0, this, 0);
}

void RemoteLinuxEnvironmentReader::handleConnectionFailure()
{
    if (m_stop)
        return;

    disconnect(m_remoteProcessRunner, 0, this, 0);
    emit error(tr("Connection error: %1").arg(m_remoteProcessRunner->lastConnectionErrorString()));
    emit finished();
}

void RemoteLinuxEnvironmentReader::handleCurrentDeviceConfigChanged()
{
    if (m_remoteProcessRunner)
        disconnect(m_remoteProcessRunner, 0, this, 0);
    m_env.clear();
    setFinished();
}

void RemoteLinuxEnvironmentReader::remoteProcessFinished(int exitCode)
{
    Q_ASSERT(exitCode == QSsh::SshRemoteProcess::FailedToStart
        || exitCode == QSsh::SshRemoteProcess::CrashExit
        || exitCode == QSsh::SshRemoteProcess::NormalExit);

    if (m_stop)
        return;

    disconnect(m_remoteProcessRunner, 0, this, 0);
    m_env.clear();
    if (exitCode == QSsh::SshRemoteProcess::NormalExit) {
        QString remoteOutput = QString::fromUtf8(m_remoteProcessRunner->readAllStandardOutput());
        if (!remoteOutput.isEmpty()) {
            m_env = Utils::Environment(remoteOutput.split(QLatin1Char('\n'),
                QString::SkipEmptyParts));
        }
    } else {
        QString errorMsg = tr("Error running remote process: %1")
            .arg(m_remoteProcessRunner->processErrorString());
        QString remoteStderr = QString::fromUtf8(m_remoteProcessRunner->readAllStandardError());
        if (!remoteStderr.isEmpty())
            errorMsg += tr("\nRemote stderr was: '%1'").arg(remoteStderr);
        emit error(errorMsg);
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
