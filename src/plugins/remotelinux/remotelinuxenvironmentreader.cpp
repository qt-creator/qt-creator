/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#include "remotelinuxenvironmentreader.h"

#include "linuxdeviceconfiguration.h"
#include "remotelinuxrunconfiguration.h"

#include <utils/ssh/sshremoteprocessrunner.h>

namespace RemoteLinux {
namespace Internal {

RemoteLinuxEnvironmentReader::RemoteLinuxEnvironmentReader(RemoteLinuxRunConfiguration *config,
        QObject *parent)
    : QObject(parent)
    , m_stop(false)
    , m_devConfig(config->deviceConfig())
    , m_runConfig(config)
{
    connect(config, SIGNAL(deviceConfigurationChanged(ProjectExplorer::Target*)),
        this, SLOT(handleCurrentDeviceConfigChanged()));
}

RemoteLinuxEnvironmentReader::~RemoteLinuxEnvironmentReader()
{
}

void RemoteLinuxEnvironmentReader::start(const QString &environmentSetupCommand)
{
    if (!m_devConfig)
        return;
    m_stop = false;
    if (!m_remoteProcessRunner
        || m_remoteProcessRunner->connection()->state() != Utils::SshConnection::Connected
        || m_remoteProcessRunner->connection()->connectionParameters() != m_devConfig->sshParameters()) {
        m_remoteProcessRunner
            = Utils::SshRemoteProcessRunner::create(m_devConfig->sshParameters());
    }
    connect(m_remoteProcessRunner.data(),
        SIGNAL(connectionError(Utils::SshError)), this,
        SLOT(handleConnectionFailure()));
    connect(m_remoteProcessRunner.data(), SIGNAL(processClosed(int)), this,
        SLOT(remoteProcessFinished(int)));
    connect(m_remoteProcessRunner.data(),
        SIGNAL(processOutputAvailable(QByteArray)), this,
        SLOT(remoteOutput(QByteArray)));
    connect(m_remoteProcessRunner.data(),
        SIGNAL(processErrorOutputAvailable(QByteArray)), this,
        SLOT(remoteErrorOutput(QByteArray)));
    const QByteArray remoteCall
        = QString(environmentSetupCommand + QLatin1String("; env")).toUtf8();
    m_remoteOutput.clear();
    m_remoteProcessRunner->run(remoteCall);
}

void RemoteLinuxEnvironmentReader::stop()
{
    m_stop = true;
    if (m_remoteProcessRunner)
        disconnect(m_remoteProcessRunner.data(), 0, this, 0);
}

void RemoteLinuxEnvironmentReader::handleConnectionFailure()
{
    if (m_stop)
        return;

    disconnect(m_remoteProcessRunner.data(), 0, this, 0);
    emit error(tr("Connection error: %1")
        .arg(m_remoteProcessRunner->connection()->errorString()));
    emit finished();
}

void RemoteLinuxEnvironmentReader::handleCurrentDeviceConfigChanged()
{
    m_devConfig = m_runConfig->deviceConfig();

    if (m_remoteProcessRunner)
        disconnect(m_remoteProcessRunner.data(), 0, this, 0);
    m_env.clear();
    setFinished();
}

void RemoteLinuxEnvironmentReader::remoteProcessFinished(int exitCode)
{
    Q_ASSERT(exitCode == Utils::SshRemoteProcess::FailedToStart
        || exitCode == Utils::SshRemoteProcess::KilledBySignal
        || exitCode == Utils::SshRemoteProcess::ExitedNormally);

    if (m_stop)
        return;

    disconnect(m_remoteProcessRunner.data(), 0, this, 0);
    m_env.clear();
    if (exitCode == Utils::SshRemoteProcess::ExitedNormally) {
        if (!m_remoteOutput.isEmpty()) {
            m_env = Utils::Environment(m_remoteOutput.split(QLatin1Char('\n'),
                QString::SkipEmptyParts));
        }
    } else {
        QString errorMsg = tr("Error running remote process: %1")
            .arg(m_remoteProcessRunner->process()->errorString());
        if (!m_remoteErrorOutput.isEmpty()) {
            errorMsg += tr("\nRemote stderr was: '%1'")
                .arg(QString::fromUtf8(m_remoteErrorOutput));
        }
        emit error(errorMsg);
    }
    setFinished();
}

void RemoteLinuxEnvironmentReader::remoteOutput(const QByteArray &data)
{
    m_remoteOutput.append(QString::fromUtf8(data));
}

void RemoteLinuxEnvironmentReader::remoteErrorOutput(const QByteArray &data)
{
    m_remoteErrorOutput += data;
}

void RemoteLinuxEnvironmentReader::setFinished()
{
    stop();
    emit finished();
}

} // namespace Internal
} // namespace RemoteLinux
