/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "remotelinuxenvironmentreader.h"

#include "linuxdeviceconfiguration.h"
#include "remotelinuxrunconfiguration.h"

#include <ssh/sshremoteprocessrunner.h>

namespace RemoteLinux {
namespace Internal {

RemoteLinuxEnvironmentReader::RemoteLinuxEnvironmentReader(RemoteLinuxRunConfiguration *config,
        QObject *parent)
    : QObject(parent)
    , m_stop(false)
    , m_devConfig(config->deviceConfig())
    , m_runConfig(config)
    , m_remoteProcessRunner(0)
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
    if (!m_remoteProcessRunner)
        m_remoteProcessRunner = new QSsh::SshRemoteProcessRunner(this);
    connect(m_remoteProcessRunner, SIGNAL(connectionError()), SLOT(handleConnectionFailure()));
    connect(m_remoteProcessRunner, SIGNAL(processClosed(int)), SLOT(remoteProcessFinished(int)));
    connect(m_remoteProcessRunner, SIGNAL(processOutputAvailable(QByteArray)),
        SLOT(remoteOutput(QByteArray)));
    connect(m_remoteProcessRunner, SIGNAL(processErrorOutputAvailable(QByteArray)),
        SLOT(remoteErrorOutput(QByteArray)));
    const QByteArray remoteCall
        = QString(environmentSetupCommand + QLatin1String("; env")).toUtf8();
    m_remoteOutput.clear();
    m_remoteProcessRunner->run(remoteCall, m_devConfig->sshParameters());
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
    m_devConfig = m_runConfig->deviceConfig();

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
        if (!m_remoteOutput.isEmpty()) {
            m_env = Utils::Environment(m_remoteOutput.split(QLatin1Char('\n'),
                QString::SkipEmptyParts));
        }
    } else {
        QString errorMsg = tr("Error running remote process: %1")
            .arg(m_remoteProcessRunner->processErrorString());
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
