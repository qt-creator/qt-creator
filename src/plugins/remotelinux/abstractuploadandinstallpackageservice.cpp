/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "abstractuploadandinstallpackageservice.h"

#include "packageuploader.h"
#include "remotelinuxpackageinstaller.h"

#include <projectexplorer/deployablefile.h>
#include <utils/qtcassert.h>

#include <QString>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {
namespace {
enum State { Inactive, Uploading, Installing };
} // anonymous namespace

class AbstractUploadAndInstallPackageServicePrivate
{
public:
    AbstractUploadAndInstallPackageServicePrivate()
        : state(Inactive), uploader(new PackageUploader)
    {
    }
    ~AbstractUploadAndInstallPackageServicePrivate() { delete uploader; }

    State state;
    PackageUploader * const uploader;
    QString packageFilePath;
};

} // namespace Internal

using namespace Internal;

AbstractUploadAndInstallPackageService::AbstractUploadAndInstallPackageService(QObject *parent)
    : AbstractRemoteLinuxDeployService(parent),
      d(new AbstractUploadAndInstallPackageServicePrivate)
{
}

AbstractUploadAndInstallPackageService::~AbstractUploadAndInstallPackageService()
{
    delete d;
}

void AbstractUploadAndInstallPackageService::setPackageFilePath(const QString &filePath)
{
    d->packageFilePath = filePath;
}

QString AbstractUploadAndInstallPackageService::packageFilePath() const
{
    return d->packageFilePath;
}

QString AbstractUploadAndInstallPackageService::uploadDir() const
{
    return QLatin1String("/tmp");
}

bool AbstractUploadAndInstallPackageService::isDeploymentNecessary() const
{
    return hasChangedSinceLastDeployment(DeployableFile(packageFilePath(), QString()));
}

void AbstractUploadAndInstallPackageService::doDeviceSetup()
{
    QTC_ASSERT(d->state == Inactive, return);

    handleDeviceSetupDone(true);
}

void AbstractUploadAndInstallPackageService::stopDeviceSetup()
{
    QTC_ASSERT(d->state == Inactive, return);

    handleDeviceSetupDone(false);
}

void AbstractUploadAndInstallPackageService::doDeploy()
{
    QTC_ASSERT(d->state == Inactive, return);

    d->state = Uploading;
    const QString fileName = Utils::FileName::fromString(packageFilePath()).fileName();
    const QString remoteFilePath = uploadDir() + QLatin1Char('/') + fileName;
    connect(d->uploader, &PackageUploader::progress,
            this, &AbstractUploadAndInstallPackageService::progressMessage);
    connect(d->uploader, &PackageUploader::uploadFinished,
            this, &AbstractUploadAndInstallPackageService::handleUploadFinished);
    d->uploader->uploadPackage(connection(), packageFilePath(), remoteFilePath);
}

void AbstractUploadAndInstallPackageService::stopDeployment()
{
    switch (d->state) {
    case Inactive:
        qWarning("%s: Unexpected state 'Inactive'.", Q_FUNC_INFO);
        break;
    case Uploading:
        d->uploader->cancelUpload();
        setFinished();
        break;
    case Installing:
        packageInstaller()->cancelInstallation();
        setFinished();
        break;
    }
}

void AbstractUploadAndInstallPackageService::handleUploadFinished(const QString &errorMsg)
{
    QTC_ASSERT(d->state == Uploading, return);

    if (!errorMsg.isEmpty()) {
        emit errorMessage(errorMsg);
        setFinished();
        return;
    }

    emit progressMessage(tr("Successfully uploaded package file."));
    const QString remoteFilePath = uploadDir() + QLatin1Char('/')
        + Utils::FileName::fromString(packageFilePath()).fileName();
    d->state = Installing;
    emit progressMessage(tr("Installing package to device..."));
    connect(packageInstaller(), SIGNAL(stdoutData(QString)), SIGNAL(stdOutData(QString)));
    connect(packageInstaller(), SIGNAL(stderrData(QString)), SIGNAL(stdErrData(QString)));
    connect(packageInstaller(), SIGNAL(finished(QString)),
        SLOT(handleInstallationFinished(QString)));
    packageInstaller()->installPackage(deviceConfiguration(), remoteFilePath, true);
}

void AbstractUploadAndInstallPackageService::handleInstallationFinished(const QString &errorMsg)
{
    QTC_ASSERT(d->state == Installing, return);

    if (errorMsg.isEmpty()) {
        saveDeploymentTimeStamp(DeployableFile(packageFilePath(), QString()));
        emit progressMessage(tr("Package installed."));
    } else {
        emit errorMessage(errorMsg);
    }
    setFinished();
}

void AbstractUploadAndInstallPackageService::setFinished()
{
    d->state = Inactive;
    disconnect(d->uploader, 0, this, 0);
    disconnect(packageInstaller(), 0, this, 0);
    handleDeploymentDone();
}

} // namespace RemoteLinux
