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
#include "maddeuploadandinstallpackagesteps.h"

#include "maemopackagecreationstep.h"
#include "maemopackageinstaller.h"
#include "maemoqemumanager.h"
#include "qt4maemodeployconfiguration.h"

#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qtsupport/baseqtversion.h>
#include <remotelinux/abstractuploadandinstallpackageservice.h>
#include <remotelinux/linuxdeviceconfiguration.h>

using namespace RemoteLinux;

namespace Madde {
namespace Internal {
namespace {
class AbstractMaddeUploadAndInstallPackageAction : public AbstractUploadAndInstallPackageService
{
    Q_OBJECT

protected:
    explicit AbstractMaddeUploadAndInstallPackageAction(AbstractRemoteLinuxDeployStep *step)
        : AbstractUploadAndInstallPackageService(step)
    {
    }

    void doDeviceSetup()
    {
        if (deviceConfiguration()->deviceType() == LinuxDeviceConfiguration::Hardware) {
            handleDeviceSetupDone(true);
            return;
        }

        if (MaemoQemuManager::instance().qemuIsRunning()) {
            handleDeviceSetupDone(true);
            return;
        }

        MaemoQemuRuntime rt;
        const int qtId = qt4BuildConfiguration() && qt4BuildConfiguration()->qtVersion()
            ? qt4BuildConfiguration()->qtVersion()->uniqueId() : -1;
        if (MaemoQemuManager::instance().runtimeForQtVersion(qtId, &rt)) {
            MaemoQemuManager::instance().startRuntime();
            emit errorMessage(tr("Cannot deploy: Qemu was not running. "
                "It has now been started up for you, but it will take "
                "a bit of time until it is ready. Please try again then."));
        } else {
            emit errorMessage(tr("Cannot deploy: You want to deploy to Qemu, but it is not enabled "
                "for this Qt version."));
        }
        handleDeviceSetupDone(false);
    }
};

class MaemoUploadAndInstallPackageAction : public AbstractMaddeUploadAndInstallPackageAction
{
    Q_OBJECT

public:
    MaemoUploadAndInstallPackageAction(AbstractRemoteLinuxDeployStep *step)
        : AbstractMaddeUploadAndInstallPackageAction(step),
          m_installer(new MaemoDebianPackageInstaller(this))
    {
    }

    AbstractRemoteLinuxPackageInstaller *packageInstaller() const { return m_installer; }

private:
    MaemoDebianPackageInstaller * const m_installer;
};

class MeegoUploadAndInstallPackageAction : public AbstractMaddeUploadAndInstallPackageAction
{
    Q_OBJECT

public:
    MeegoUploadAndInstallPackageAction(AbstractRemoteLinuxDeployStep *step)
        : AbstractMaddeUploadAndInstallPackageAction(step),
          m_installer(new MaemoRpmPackageInstaller(this))
    {
    }

    AbstractRemoteLinuxPackageInstaller *packageInstaller() const { return m_installer; }

private:
    MaemoRpmPackageInstaller * const m_installer;
};

} // anonymous namespace


MaemoUploadAndInstallPackageStep::MaemoUploadAndInstallPackageStep(ProjectExplorer::BuildStepList *bsl)
    : AbstractRemoteLinuxDeployStep(bsl, stepId())
{
    ctor();
}

MaemoUploadAndInstallPackageStep::MaemoUploadAndInstallPackageStep(ProjectExplorer::BuildStepList *bsl,
    MaemoUploadAndInstallPackageStep *other) : AbstractRemoteLinuxDeployStep(bsl, other)
{
    ctor();
}

void MaemoUploadAndInstallPackageStep::ctor()
{
    setDefaultDisplayName(displayName());
    m_deployService = new MaemoUploadAndInstallPackageAction(this);
}

AbstractRemoteLinuxDeployService *MaemoUploadAndInstallPackageStep::deployService() const
{
    return m_deployService;
}

bool MaemoUploadAndInstallPackageStep::isDeploymentPossible(QString *whyNot) const
{
    const AbstractMaemoPackageCreationStep * const pStep
        = deployConfiguration()->earlierBuildStep<MaemoDebianPackageCreationStep>(this);
    if (!pStep) {
        if (whyNot)
            *whyNot = tr("No Debian package creation step found.");
        return false;
    }
    m_deployService->setPackageFilePath(pStep->packageFilePath());
    return AbstractRemoteLinuxDeployStep::isDeploymentPossible(whyNot);
}

QString MaemoUploadAndInstallPackageStep::stepId()
{
    return QLatin1String("MaemoUploadAndInstallDpkgPackageStep");
}

QString MaemoUploadAndInstallPackageStep::displayName()
{
    return tr("Deploy Debian package via SFTP upload");
}


MeegoUploadAndInstallPackageStep::MeegoUploadAndInstallPackageStep(ProjectExplorer::BuildStepList *bsl)
    : AbstractRemoteLinuxDeployStep(bsl, stepId())
{
    ctor();
}

MeegoUploadAndInstallPackageStep::MeegoUploadAndInstallPackageStep(ProjectExplorer::BuildStepList *bsl,
    MeegoUploadAndInstallPackageStep *other) : AbstractRemoteLinuxDeployStep(bsl, other)
{
    ctor();
}

void MeegoUploadAndInstallPackageStep::ctor()
{
    setDefaultDisplayName(displayName());
    m_deployService = new MeegoUploadAndInstallPackageAction(this);
}

AbstractRemoteLinuxDeployService *MeegoUploadAndInstallPackageStep::deployService() const
{
    return m_deployService;
}

bool MeegoUploadAndInstallPackageStep::isDeploymentPossible(QString *whyNot) const
{
    const AbstractMaemoPackageCreationStep * const pStep
        = deployConfiguration()->earlierBuildStep<MaemoRpmPackageCreationStep>(this);
    if (!pStep) {
        if (whyNot)
            *whyNot = tr("No RPM package creation step found.");
        return false;
    }
    m_deployService->setPackageFilePath(pStep->packageFilePath());
    return AbstractRemoteLinuxDeployStep::isDeploymentPossible(whyNot);
}

QString MeegoUploadAndInstallPackageStep::stepId()
{
    return QLatin1String("MaemoUploadAndInstallRpmPackageStep");
}

QString MeegoUploadAndInstallPackageStep::displayName()
{
    return tr("Deploy RPM package via SFTP upload");
}

} // namespace Internal
} // namespace Madde

#include "maddeuploadandinstallpackagesteps.moc"
