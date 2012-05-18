/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "remotelinuxpackageinstaller.h"

#include "linuxdeviceconfiguration.h"

#include <utils/qtcassert.h>
#include <ssh/sshremoteprocessrunner.h>

#include <QByteArray>

using namespace QSsh;

namespace RemoteLinux {
namespace Internal {

class AbstractRemoteLinuxPackageInstallerPrivate
{
public:
    AbstractRemoteLinuxPackageInstallerPrivate() : isRunning(false), installer(0), killProcess(0) {}

    bool isRunning;
    LinuxDeviceConfiguration::ConstPtr deviceConfig;
    QSsh::SshRemoteProcessRunner *installer;
    QSsh::SshRemoteProcessRunner *killProcess;
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

void AbstractRemoteLinuxPackageInstaller::installPackage(const LinuxDeviceConfiguration::ConstPtr &deviceConfig,
    const QString &packageFilePath, bool removePackageFile)
{
    QTC_ASSERT(!d->isRunning, return);

    d->deviceConfig = deviceConfig;
    prepareInstallation();
    if (!d->installer)
        d->installer = new SshRemoteProcessRunner(this);
    connect(d->installer, SIGNAL(connectionError()), SLOT(handleConnectionError()));
    connect(d->installer, SIGNAL(processOutputAvailable(QByteArray)),
        SLOT(handleInstallerOutput(QByteArray)));
    connect(d->installer, SIGNAL(processErrorOutputAvailable(QByteArray)),
        SLOT(handleInstallerErrorOutput(QByteArray)));
    connect(d->installer, SIGNAL(processClosed(int)), SLOT(handleInstallationFinished(int)));

    QString cmdLine = installCommandLine(packageFilePath);
    if (removePackageFile)
        cmdLine += QLatin1String(" && (rm ") + packageFilePath + QLatin1String(" || :)");
    d->installer->run(cmdLine.toUtf8(), deviceConfig->sshParameters());
    d->isRunning = true;
}

void AbstractRemoteLinuxPackageInstaller::cancelInstallation()
{
    QTC_ASSERT(d->installer && d->isRunning, return);

    if (!d->killProcess)
        d->killProcess = new SshRemoteProcessRunner(this);
    d->killProcess->run(cancelInstallationCommandLine().toUtf8(), d->deviceConfig->sshParameters());
    setFinished();
}

void AbstractRemoteLinuxPackageInstaller::handleConnectionError()
{
    if (!d->isRunning)
        return;
    emit finished(tr("Connection failure: %1").arg(d->installer->lastConnectionErrorString()));
    setFinished();
}

void AbstractRemoteLinuxPackageInstaller::handleInstallationFinished(int exitStatus)
{
    if (!d->isRunning)
        return;

    if (exitStatus != SshRemoteProcess::ExitedNormally || d->installer->processExitCode() != 0) {
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
    disconnect(d->installer, 0, this, 0);
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
