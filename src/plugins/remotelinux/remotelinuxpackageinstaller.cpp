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

#include <projectexplorer/devicesupport/idevice.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace QSsh;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

class AbstractRemoteLinuxPackageInstallerPrivate
{
public:
    IDevice::ConstPtr m_device;
    QtcProcess m_installer;
    QtcProcess m_killer;
};

} // namespace Internal

AbstractRemoteLinuxPackageInstaller::AbstractRemoteLinuxPackageInstaller(QObject *parent)
    : QObject(parent), d(new Internal::AbstractRemoteLinuxPackageInstallerPrivate)
{
    connect(&d->m_installer, &QtcProcess::readyReadStandardOutput, this, [this] {
        emit stdoutData(QString::fromUtf8(d->m_installer.readAllStandardOutput()));
    });
    connect(&d->m_installer, &QtcProcess::readyReadStandardError, this, [this] {
        emit stderrData(QString::fromUtf8(d->m_installer.readAllStandardError()));
    });
    connect(&d->m_installer, &QtcProcess::finished, this, [this] {
        const QString errorMessage = d->m_installer.result() == ProcessResult::FinishedWithSuccess
                ? QString() : tr("Installing package failed.") + d->m_installer.errorString();
        emit finished(errorMessage);
    });
}

AbstractRemoteLinuxPackageInstaller::~AbstractRemoteLinuxPackageInstaller() = default;

void AbstractRemoteLinuxPackageInstaller::installPackage(const IDevice::ConstPtr &deviceConfig,
    const QString &packageFilePath, bool removePackageFile)
{
    QTC_ASSERT(d->m_installer.state() == QProcess::NotRunning, return);

    d->m_device = deviceConfig;

    QString cmdLine = installCommandLine(packageFilePath);
    if (removePackageFile)
        cmdLine += QLatin1String(" && (rm ") + packageFilePath + QLatin1String(" || :)");
    d->m_installer.setCommand({d->m_device->mapToGlobalPath("/bin/sh"), {"-c", cmdLine}});
    d->m_installer.start();
}

void AbstractRemoteLinuxPackageInstaller::cancelInstallation()
{
    QTC_ASSERT(d->m_installer.state() != QProcess::NotRunning, return);

    d->m_killer.setCommand({d->m_device->mapToGlobalPath("/bin/sh"),
                            {"-c", cancelInstallationCommandLine()}});
    d->m_killer.start();
    d->m_installer.close();
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
