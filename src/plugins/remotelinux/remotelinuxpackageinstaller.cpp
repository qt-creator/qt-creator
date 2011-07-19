/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
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
#include "remotelinuxpackageinstaller.h"

#include <QtCore/QByteArray>

#include <utils/qtcassert.h>
#include <utils/ssh/sshconnection.h>
#include <utils/ssh/sshremoteprocessrunner.h>

using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class AbstractRemoteLinuxPackageInstallerPrivate
{
public:
    AbstractRemoteLinuxPackageInstallerPrivate() : isRunning(false) {}

    bool isRunning;
    Utils::SshRemoteProcessRunner::Ptr installer;
};

} // namespace Internal

AbstractRemoteLinuxPackageInstaller::AbstractRemoteLinuxPackageInstaller(QObject *parent)
    : QObject(parent), m_d(new Internal::AbstractRemoteLinuxPackageInstallerPrivate)
{
}

AbstractRemoteLinuxPackageInstaller::~AbstractRemoteLinuxPackageInstaller()
{
    delete m_d;
}

void AbstractRemoteLinuxPackageInstaller::installPackage(const SshConnection::Ptr &connection,
    const QString &packageFilePath, bool removePackageFile)
{
    QTC_ASSERT(connection && connection->state() == SshConnection::Connected
        && !m_d->isRunning, return);

    prepareInstallation();
    m_d->installer = SshRemoteProcessRunner::create(connection);
    connect(m_d->installer.data(), SIGNAL(connectionError(Utils::SshError)),
        SLOT(handleConnectionError()));
    connect(m_d->installer.data(), SIGNAL(processOutputAvailable(QByteArray)),
        SLOT(handleInstallerOutput(QByteArray)));
    connect(m_d->installer.data(), SIGNAL(processErrorOutputAvailable(QByteArray)),
        SLOT(handleInstallerErrorOutput(QByteArray)));
    connect(m_d->installer.data(), SIGNAL(processClosed(int)), SLOT(handleInstallationFinished(int)));

    QString cmdLine = installCommandLine(packageFilePath);
    if (removePackageFile)
        cmdLine += QLatin1String(" && (rm ") + packageFilePath + QLatin1String(" || :)");
    m_d->installer->run(cmdLine.toUtf8());
    m_d->isRunning = true;
}

void AbstractRemoteLinuxPackageInstaller::cancelInstallation()
{
    QTC_ASSERT(m_d->installer && m_d->installer->connection()->state() == SshConnection::Connected
        && m_d->isRunning, return);

    const SshRemoteProcessRunner::Ptr killProcess
        = SshRemoteProcessRunner::create(m_d->installer->connection());
    killProcess->run(cancelInstallationCommandLine().toUtf8());
    setFinished();
}

void AbstractRemoteLinuxPackageInstaller::handleConnectionError()
{
    if (!m_d->isRunning)
        return;
    emit finished(tr("Connection failure: %1").arg(m_d->installer->connection()->errorString()));
    setFinished();
}

void AbstractRemoteLinuxPackageInstaller::handleInstallationFinished(int exitStatus)
{
    if (!m_d->isRunning)
        return;

    if (exitStatus != SshRemoteProcess::ExitedNormally
            || m_d->installer->process()->exitCode() != 0) {
        emit finished(tr("Installing package failed."));
    } else if (!errorString().isEmpty()) {
        emit finished(errorString());
    } else {
        emit finished();
    }

    setFinished();
}

void AbstractRemoteLinuxPackageInstaller::handleInstallerOutput(const QByteArray &output)
{
    emit stdoutData(QString::fromUtf8(output));
}

void AbstractRemoteLinuxPackageInstaller::handleInstallerErrorOutput(const QByteArray &output)
{
    emit stderrData(QString::fromUtf8(output));
}

void AbstractRemoteLinuxPackageInstaller::setFinished()
{
    disconnect(m_d->installer.data(), 0, this, 0);
    m_d->installer.clear();
    m_d->isRunning = false;
}


RemoteLinuxTarPackageInstaller::RemoteLinuxTarPackageInstaller(QObject *parent)
    : AbstractRemoteLinuxPackageInstaller(parent)
{
}

QString RemoteLinuxTarPackageInstaller::installCommandLine(const QString &packageFilePath) const
{
    return QLatin1String("cd / && tar xvf ") + packageFilePath;
}

QString RemoteLinuxTarPackageInstaller::cancelInstallationCommandLine() const
{
    return QLatin1String("pkill tar");
}

} // namespace RemoteLinux
