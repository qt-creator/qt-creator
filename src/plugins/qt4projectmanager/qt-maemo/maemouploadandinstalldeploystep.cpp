/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "maemouploadandinstalldeploystep.h"

#include "maemoglobal.h"
#include "maemopackagecreationstep.h"
#include "maemopackageinstaller.h"
#include "maemopackageuploader.h"
#include "qt4maemotarget.h"

#include <qt4projectmanager/qt4buildconfiguration.h>

#include <QtCore/QFileInfo>

#define ASSERT_BASE_STATE(state) ASSERT_STATE_GENERIC(BaseState, state, baseState())
#define ASSERT_STATE(state) ASSERT_STATE_GENERIC(ExtendedState, state, m_extendedState)

using namespace ProjectExplorer;
using namespace Utils;

namespace Qt4ProjectManager {
namespace Internal {

MaemoUploadAndInstallDeployStep::MaemoUploadAndInstallDeployStep(BuildStepList *parent)
    : AbstractMaemoDeployStep(parent, Id)
{
    ctor();
}

MaemoUploadAndInstallDeployStep::MaemoUploadAndInstallDeployStep(BuildStepList *parent,
    MaemoUploadAndInstallDeployStep *other)
    : AbstractMaemoDeployStep(parent, other)
{
    ctor();
}

void MaemoUploadAndInstallDeployStep::ctor()
{
    setDefaultDisplayName(DisplayName);

    m_extendedState = Inactive;

    m_uploader = new MaemoPackageUploader(this);
    connect(m_uploader, SIGNAL(progress(QString)),
        SLOT(handleProgressReport(QString)));
    connect(m_uploader, SIGNAL(uploadFinished(QString)),
        SLOT(handleUploadFinished(QString)));

    if (qobject_cast<AbstractDebBasedQt4MaemoTarget *>(target()))
        m_installer = new MaemoDebianPackageInstaller(this);
    else if (qobject_cast<AbstractRpmBasedQt4MaemoTarget *>(target()))
        m_installer = new MaemoRpmPackageInstaller(this);
    else
        m_installer = new MaemoTarPackageInstaller(this);
    connect(m_installer, SIGNAL(stdout(QString)),
        SLOT(handleRemoteStdout(QString)));
    connect(m_installer, SIGNAL(stderr(QString)),
        SLOT(handleRemoteStderr(QString)));
    connect(m_installer, SIGNAL(finished(QString)),
        SLOT(handleInstallationFinished(QString)));
}

bool MaemoUploadAndInstallDeployStep::isDeploymentPossibleInternal(QString &whyNot) const
{
    if (!packagingStep()) {
        whyNot = tr("No packaging step found.");
        return false;
    }
    return true;
}

bool MaemoUploadAndInstallDeployStep::isDeploymentNeeded(const QString &hostName) const
{
    const AbstractMaemoPackageCreationStep * const pStep = packagingStep();
    Q_ASSERT(pStep);
    const MaemoDeployable d(pStep->packageFilePath(), QString());
    return currentlyNeedsDeployment(hostName, d);
}

void MaemoUploadAndInstallDeployStep::startInternal()
{
    Q_ASSERT(m_extendedState == Inactive);

    upload();
}

void MaemoUploadAndInstallDeployStep::stopInternal()
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

void MaemoUploadAndInstallDeployStep::upload()
{
    m_extendedState = Uploading;
    const QString localFilePath = packagingStep()->packageFilePath();
    const QString fileName = QFileInfo(localFilePath).fileName();
    const QString remoteFilePath = uploadDir() + QLatin1Char('/') + fileName;
    m_uploader->uploadPackage(connection(), localFilePath, remoteFilePath);
}

void MaemoUploadAndInstallDeployStep::handleUploadFinished(const QString &errorMsg)
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
        m_installer->installPackage(connection(), remoteFilePath, true);
    }
}

void MaemoUploadAndInstallDeployStep::handleInstallationFinished(const QString &errorMsg)
{
    ASSERT_BASE_STATE(QList<BaseState>() << Deploying << StopRequested);
    ASSERT_STATE(QList<ExtendedState>() << Installing << Inactive);

    if (m_extendedState == Inactive)
        return;

    if (errorMsg.isEmpty()) {
        setDeployed(connection()->connectionParameters().host,
            MaemoDeployable(packagingStep()->packageFilePath(), QString()));
        writeOutput(tr("Package installed."));
    } else {
        raiseError(errorMsg);
    }
    setFinished();
}

void MaemoUploadAndInstallDeployStep::setFinished()
{
    m_extendedState = Inactive;
    setDeploymentFinished();
}

QString MaemoUploadAndInstallDeployStep::uploadDir() const
{
    return MaemoGlobal::homeDirOnDevice(connection()->connectionParameters().userName);
}

const QString MaemoUploadAndInstallDeployStep::Id("MaemoUploadAndInstallDeployStep");
const QString MaemoUploadAndInstallDeployStep::DisplayName
    = tr("Deploy package via SFTP upload");

} // namespace Internal
} // namespace Qt4ProjectManager
