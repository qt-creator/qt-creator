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

#include "maemouploadandinstalldeploystep.h"

#include "deployablefile.h"
#include "maemoglobal.h"
#include "maemopackagecreationstep.h"
#include "maemopackageinstaller.h"
#include "maemopackageuploader.h"
#include "qt4maemodeployconfiguration.h"
#include "qt4maemotarget.h"

#include <qt4projectmanager/qt4buildconfiguration.h>

#include <QtCore/QFileInfo>

#define ASSERT_BASE_STATE(state) ASSERT_STATE_GENERIC(BaseState, state, baseState())
#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(ExtendedState, state, m_extendedState)

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

AbstractMaemoUploadAndInstallStep::AbstractMaemoUploadAndInstallStep(BuildStepList *parent, const QString &id)
    : AbstractMaemoDeployStep(parent, id)
{
}

AbstractMaemoUploadAndInstallStep::AbstractMaemoUploadAndInstallStep(BuildStepList *parent,
    AbstractMaemoUploadAndInstallStep *other)
    : AbstractMaemoDeployStep(parent, other)
{
}

void AbstractMaemoUploadAndInstallStep::finishInitialization(const QString &displayName,
    AbstractMaemoPackageInstaller *installer)
{
    setDefaultDisplayName(displayName);
    m_installer = installer;
    m_extendedState = Inactive;

    m_uploader = new MaemoPackageUploader(this);
    connect(m_uploader, SIGNAL(progress(QString)),
        SLOT(handleProgressReport(QString)));
    connect(m_uploader, SIGNAL(uploadFinished(QString)),
        SLOT(handleUploadFinished(QString)));
    connect(m_installer, SIGNAL(stdoutData(QString)),
        SLOT(handleRemoteStdout(QString)));
    connect(m_installer, SIGNAL(stderrData(QString)),
        SLOT(handleRemoteStderr(QString)));
    connect(m_installer, SIGNAL(finished(QString)),
        SLOT(handleInstallationFinished(QString)));
}

bool AbstractMaemoUploadAndInstallStep::isDeploymentPossibleInternal(QString &whyNot) const
{
    if (!packagingStep()) {
        whyNot = tr("No matching packaging step found.");
        return false;
    }
    return true;
}

bool AbstractMaemoUploadAndInstallStep::isDeploymentNeeded(const QString &hostName) const
{
    const AbstractMaemoPackageCreationStep * const pStep = packagingStep();
    Q_ASSERT(pStep);
    const DeployableFile d(pStep->packageFilePath(), QString());
    return currentlyNeedsDeployment(hostName, d);
}

void AbstractMaemoUploadAndInstallStep::startInternal()
{
    Q_ASSERT(m_extendedState == Inactive);

    upload();
}

void AbstractMaemoUploadAndInstallStep::stopInternal()
{
    ASSERT_BASE_STATE(StopRequested);
    ASSERT_STATE(QList<ExtendedState>() << Uploading << Installing);

    switch (m_extendedState) {
    case Uploading:
        m_uploader->cancelUpload();
        break;
    case Installing:
        m_installer->cancelInstallation();
        break;
    case Inactive:
        break;
    default:
        qFatal("Missing switch case in %s.", Q_FUNC_INFO);

    }
    setFinished();
}

void AbstractMaemoUploadAndInstallStep::upload()
{
    m_extendedState = Uploading;
    const QString localFilePath = packagingStep()->packageFilePath();
    const QString fileName = QFileInfo(localFilePath).fileName();
    const QString remoteFilePath = uploadDir() + QLatin1Char('/') + fileName;
    m_uploader->uploadPackage(connection(), localFilePath, remoteFilePath);
}

void AbstractMaemoUploadAndInstallStep::handleUploadFinished(const QString &errorMsg)
{
    ASSERT_BASE_STATE(QList<BaseState>() << Deploying << StopRequested);
    ASSERT_STATE(QList<ExtendedState>() << Uploading << Inactive);

    if (m_extendedState == Inactive)
        return;

    if (!errorMsg.isEmpty()) {
        raiseError(errorMsg);
        setFinished();
    } else {
        writeOutput(tr("Successfully uploaded package file."));
        const QString remoteFilePath = uploadDir() + QLatin1Char('/')
            + QFileInfo(packagingStep()->packageFilePath()).fileName();
        m_extendedState = Installing;
        writeOutput(tr("Installing package to device..."));
        m_installer->installPackage(connection(), deviceConfiguration(), remoteFilePath, true);
    }
}

void AbstractMaemoUploadAndInstallStep::handleInstallationFinished(const QString &errorMsg)
{
    ASSERT_BASE_STATE(QList<BaseState>() << Deploying << StopRequested);
    ASSERT_STATE(QList<ExtendedState>() << Installing << Inactive);

    if (m_extendedState == Inactive)
        return;

    if (errorMsg.isEmpty()) {
        setDeployed(connection()->connectionParameters().host,
            DeployableFile(packagingStep()->packageFilePath(), QString()));
        writeOutput(tr("Package installed."));
    } else {
        raiseError(errorMsg);
    }
    setFinished();
}

void AbstractMaemoUploadAndInstallStep::setFinished()
{
    m_extendedState = Inactive;
    setDeploymentFinished();
}

QString AbstractMaemoUploadAndInstallStep::uploadDir() const
{
    return MaemoGlobal::homeDirOnDevice(connection()->connectionParameters().userName);
}


MaemoUploadAndInstallDpkgPackageStep::MaemoUploadAndInstallDpkgPackageStep(ProjectExplorer::BuildStepList *bc)
    : AbstractMaemoUploadAndInstallStep(bc, Id)
{
    ctor();
}

MaemoUploadAndInstallDpkgPackageStep::MaemoUploadAndInstallDpkgPackageStep(ProjectExplorer::BuildStepList *bc,
    MaemoUploadAndInstallDpkgPackageStep *other)
    : AbstractMaemoUploadAndInstallStep(bc, other)
{
    ctor();
}

void MaemoUploadAndInstallDpkgPackageStep::ctor()
{
    finishInitialization(displayName(), new MaemoDebianPackageInstaller(this));
}


const AbstractMaemoPackageCreationStep *MaemoUploadAndInstallDpkgPackageStep::packagingStep() const
{
    return MaemoGlobal::earlierBuildStep<MaemoDebianPackageCreationStep>(maemoDeployConfig(), this);
}

const QString MaemoUploadAndInstallDpkgPackageStep::Id("MaemoUploadAndInstallDpkgPackageStep");

QString MaemoUploadAndInstallDpkgPackageStep::displayName()
{
    return tr("Deploy Debian package via SFTP upload");
}

MaemoUploadAndInstallRpmPackageStep::MaemoUploadAndInstallRpmPackageStep(ProjectExplorer::BuildStepList *bc)
    : AbstractMaemoUploadAndInstallStep(bc, Id)
{
    ctor();
}

MaemoUploadAndInstallRpmPackageStep::MaemoUploadAndInstallRpmPackageStep(ProjectExplorer::BuildStepList *bc,
    MaemoUploadAndInstallRpmPackageStep *other)
    : AbstractMaemoUploadAndInstallStep(bc, other)
{
    ctor();
}

void MaemoUploadAndInstallRpmPackageStep::ctor()
{
    finishInitialization(displayName(), new MaemoRpmPackageInstaller(this));
}

const AbstractMaemoPackageCreationStep *MaemoUploadAndInstallRpmPackageStep::packagingStep() const
{
    return MaemoGlobal::earlierBuildStep<MaemoRpmPackageCreationStep>(maemoDeployConfig(), this);
}

const QString MaemoUploadAndInstallRpmPackageStep::Id("MaemoUploadAndInstallRpmPackageStep");

QString MaemoUploadAndInstallRpmPackageStep::displayName()
{
    return tr("Deploy RPM package via SFTP upload");
}

MaemoUploadAndInstallTarPackageStep::MaemoUploadAndInstallTarPackageStep(ProjectExplorer::BuildStepList *bc)
    : AbstractMaemoUploadAndInstallStep(bc, Id)
{
    ctor();
}

MaemoUploadAndInstallTarPackageStep::MaemoUploadAndInstallTarPackageStep(ProjectExplorer::BuildStepList *bc,
    MaemoUploadAndInstallTarPackageStep *other)
    : AbstractMaemoUploadAndInstallStep(bc, other)
{
    ctor();
}

const AbstractMaemoPackageCreationStep *MaemoUploadAndInstallTarPackageStep::packagingStep() const
{
    return MaemoGlobal::earlierBuildStep<MaemoTarPackageCreationStep>(maemoDeployConfig(), this);
}

void MaemoUploadAndInstallTarPackageStep::ctor()
{
    finishInitialization(displayName(), new MaemoTarPackageInstaller(this));
}

const QString MaemoUploadAndInstallTarPackageStep::Id("MaemoUploadAndInstallTarPackageStep");

QString MaemoUploadAndInstallTarPackageStep::displayName()
{
    return tr("Deploy tarball via SFTP upload");
}

} // namespace Internal
} // namespace RemoteLinux
