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

#include "abstractuploadandinstallpackageservice.h"

#include "filetransfer.h"
#include "remotelinuxpackageinstaller.h"

#include <projectexplorer/deployablefile.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>

#include <QDateTime>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {
namespace {
enum State { Inactive, Uploading, Installing };
} // anonymous namespace

class AbstractUploadAndInstallPackageServicePrivate
{
public:
    State state = Inactive;
    FileTransfer uploader;
    Utils::FilePath packageFilePath;
};

} // namespace Internal

using namespace Internal;

AbstractUploadAndInstallPackageService::AbstractUploadAndInstallPackageService()
    : d(new AbstractUploadAndInstallPackageServicePrivate)
{
    connect(&d->uploader, &FileTransfer::done, this,
            &AbstractUploadAndInstallPackageService::handleUploadFinished);
    connect(&d->uploader, &FileTransfer::progress, this,
            &AbstractUploadAndInstallPackageService::progressMessage);
}

AbstractUploadAndInstallPackageService::~AbstractUploadAndInstallPackageService()
{
    delete d;
}

void AbstractUploadAndInstallPackageService::setPackageFilePath(const FilePath &filePath)
{
    d->packageFilePath = filePath;
}

QString AbstractUploadAndInstallPackageService::uploadDir() const
{
    return QLatin1String("/tmp");
}

bool AbstractUploadAndInstallPackageService::isDeploymentNecessary() const
{
    return hasLocalFileChanged(DeployableFile(d->packageFilePath, QString()));
}

void AbstractUploadAndInstallPackageService::doDeviceSetup()
{
    QTC_ASSERT(d->state == Inactive, return);
    AbstractRemoteLinuxDeployService::doDeviceSetup();
}

void AbstractUploadAndInstallPackageService::stopDeviceSetup()
{
    QTC_ASSERT(d->state == Inactive, return);
    AbstractRemoteLinuxDeployService::stopDeviceSetup();
}

void AbstractUploadAndInstallPackageService::doDeploy()
{
    QTC_ASSERT(d->state == Inactive, return);

    d->state = Uploading;

    const QString remoteFilePath = uploadDir() + QLatin1Char('/') + d->packageFilePath.fileName();
    const FilesToTransfer files {{d->packageFilePath,
                    deviceConfiguration()->filePath(remoteFilePath)}};
    d->uploader.setDevice(deviceConfiguration());
    d->uploader.setFilesToTransfer(files);
    d->uploader.start();
}

void AbstractUploadAndInstallPackageService::stopDeployment()
{
    switch (d->state) {
    case Inactive:
        qWarning("%s: Unexpected state 'Inactive'.", Q_FUNC_INFO);
        break;
    case Uploading:
        d->uploader.stop();
        setFinished();
        break;
    case Installing:
        packageInstaller()->cancelInstallation();
        setFinished();
        break;
    }
}

void AbstractUploadAndInstallPackageService::handleUploadFinished(const ProcessResultData &resultData)
{
    QTC_ASSERT(d->state == Uploading, return);

    if (resultData.m_error != QProcess::UnknownError) {
        emit errorMessage(resultData.m_errorString);
        setFinished();
        return;
    }

    emit progressMessage(tr("Successfully uploaded package file."));
    const QString remoteFilePath = uploadDir() + '/' + d->packageFilePath.fileName();
    d->state = Installing;
    emit progressMessage(tr("Installing package to device..."));
    connect(packageInstaller(), &AbstractRemoteLinuxPackageInstaller::stdoutData,
            this, &AbstractRemoteLinuxDeployService::stdOutData);
    connect(packageInstaller(), &AbstractRemoteLinuxPackageInstaller::stderrData,
            this, &AbstractRemoteLinuxDeployService::stdErrData);
    connect(packageInstaller(), &AbstractRemoteLinuxPackageInstaller::finished,
            this, &AbstractUploadAndInstallPackageService::handleInstallationFinished);
    packageInstaller()->installPackage(deviceConfiguration(), remoteFilePath, true);
}

void AbstractUploadAndInstallPackageService::handleInstallationFinished(const QString &errorMsg)
{
    QTC_ASSERT(d->state == Installing, return);

    if (errorMsg.isEmpty()) {
        saveDeploymentTimeStamp(DeployableFile(d->packageFilePath, QString()), QDateTime());
        emit progressMessage(tr("Package installed."));
    } else {
        emit errorMessage(errorMsg);
    }
    setFinished();
}

void AbstractUploadAndInstallPackageService::setFinished()
{
    d->state = Inactive;
    d->uploader.stop();
    disconnect(packageInstaller(), nullptr, this, nullptr);
    handleDeploymentDone();
}

} // namespace RemoteLinux
