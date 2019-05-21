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

#include "remotelinuxpackageinstaller.h"

#include <utils/qtcassert.h>
#include <ssh/sshremoteprocessrunner.h>

using namespace ProjectExplorer;
using namespace QSsh;

namespace RemoteLinux {
namespace Internal {

class AbstractRemoteLinuxPackageInstallerPrivate
{
public:
    bool isRunning = false;
    IDevice::ConstPtr deviceConfig;
    SshRemoteProcessRunner *installer = nullptr;
    SshRemoteProcessRunner *killProcess = nullptr;
};

} // namespace Internal

AbstractRemoteLinuxPackageInstaller::AbstractRemoteLinuxPackageInstaller(QObject *parent)
    : QObject(parent), d(new Internal::AbstractRemoteLinuxPackageInstallerPrivate)
{
}

AbstractRemoteLinuxPackageInstaller::~AbstractRemoteLinuxPackageInstaller()
{
    delete d;
}

void AbstractRemoteLinuxPackageInstaller::installPackage(const IDevice::ConstPtr &deviceConfig,
    const QString &packageFilePath, bool removePackageFile)
{
    QTC_ASSERT(!d->isRunning, return);

    d->deviceConfig = deviceConfig;
    prepareInstallation();
    if (!d->installer)
        d->installer = new SshRemoteProcessRunner(this);
    connect(d->installer, &SshRemoteProcessRunner::connectionError,
            this, &AbstractRemoteLinuxPackageInstaller::handleConnectionError);
    connect(d->installer, &SshRemoteProcessRunner::readyReadStandardOutput,
            this, &AbstractRemoteLinuxPackageInstaller::handleInstallerOutput);
    connect(d->installer, &SshRemoteProcessRunner::readyReadStandardError,
            this, &AbstractRemoteLinuxPackageInstaller::handleInstallerErrorOutput);
    connect(d->installer, &SshRemoteProcessRunner::processClosed,
            this, &AbstractRemoteLinuxPackageInstaller::handleInstallationFinished);

    QString cmdLine = installCommandLine(packageFilePath);
    if (removePackageFile)
        cmdLine += QLatin1String(" && (rm ") + packageFilePath + QLatin1String(" || :)");
    d->installer->run(cmdLine, deviceConfig->sshParameters());
    d->isRunning = true;
}

void AbstractRemoteLinuxPackageInstaller::cancelInstallation()
{
    QTC_ASSERT(d->installer && d->isRunning, return);

    if (!d->killProcess)
        d->killProcess = new SshRemoteProcessRunner(this);
    d->killProcess->run(cancelInstallationCommandLine(), d->deviceConfig->sshParameters());
    setFinished();
}

void AbstractRemoteLinuxPackageInstaller::handleConnectionError()
{
    if (!d->isRunning)
        return;
    emit finished(tr("Connection failure: %1").arg(d->installer->lastConnectionErrorString()));
    setFinished();
}

void AbstractRemoteLinuxPackageInstaller::handleInstallationFinished(const QString &error)
{
    if (!d->isRunning)
        return;

    if (!error.isEmpty() || d->installer->processExitCode() != 0)
        emit finished(tr("Installing package failed."));
    else if (!errorString().isEmpty())
        emit finished(errorString());
    else
        emit finished();

    setFinished();
}

void AbstractRemoteLinuxPackageInstaller::handleInstallerOutput()
{
    emit stdoutData(QString::fromUtf8(d->installer->readAllStandardOutput()));
}

void AbstractRemoteLinuxPackageInstaller::handleInstallerErrorOutput()
{
    emit stderrData(QString::fromUtf8(d->installer->readAllStandardError()));
}

void AbstractRemoteLinuxPackageInstaller::setFinished()
{
    disconnect(d->installer, nullptr, this, nullptr);
    d->isRunning = false;
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
