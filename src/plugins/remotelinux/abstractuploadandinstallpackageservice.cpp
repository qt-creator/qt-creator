/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/
#include "abstractuploadandinstallpackageservice.h"

#include "deployablefile.h"
#include "linuxdeviceconfiguration.h"
#include "packageuploader.h"
#include "remotelinuxpackageinstaller.h"

#include <utils/qtcassert.h>
#include <ssh/sshconnection.h>

#include <QFileInfo>
#include <QString>

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
    const QString fileName = QFileInfo(packageFilePath()).fileName();
    const QString remoteFilePath = uploadDir() + QLatin1Char('/') + fileName;
    connect(d->uploader, SIGNAL(progress(QString)), SIGNAL(progressMessage(QString)));
    connect(d->uploader, SIGNAL(uploadFinished(QString)), SLOT(handleUploadFinished(QString)));
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
        + QFileInfo(packageFilePath()).fileName();
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
