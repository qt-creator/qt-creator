/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "uploadandinstalltarpackagestep.h"

#include "remotelinuxdeployconfiguration.h"
#include "remotelinuxpackageinstaller.h"
#include "tarpackagecreationstep.h"

#include <projectexplorer/buildsteplist.h>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

class UploadAndInstallTarPackageServicePrivate
{
public:
    RemoteLinuxTarPackageInstaller installer;
};

} // namespace Internal

using namespace Internal;

UploadAndInstallTarPackageService::UploadAndInstallTarPackageService(QObject *parent)
    : AbstractUploadAndInstallPackageService(parent),
      d(new UploadAndInstallTarPackageServicePrivate)
{
}

UploadAndInstallTarPackageService::~UploadAndInstallTarPackageService()
{
    delete d;
}

AbstractRemoteLinuxPackageInstaller *UploadAndInstallTarPackageService::packageInstaller() const
{
    return &d->installer;
}


UploadAndInstallTarPackageStep::UploadAndInstallTarPackageStep(BuildStepList *bsl)
    : AbstractRemoteLinuxDeployStep(bsl, stepId())
{
    ctor();
}

UploadAndInstallTarPackageStep::UploadAndInstallTarPackageStep(BuildStepList *bsl,
        UploadAndInstallTarPackageStep *other)
    : AbstractRemoteLinuxDeployStep(bsl, other)
{
    ctor();
}

void UploadAndInstallTarPackageStep::ctor()
{
    m_deployService = new UploadAndInstallTarPackageService(this);
    setDefaultDisplayName(displayName());
}

bool UploadAndInstallTarPackageStep::initInternal(QString *error)
{
    const TarPackageCreationStep * const pStep
        = deployConfiguration()->earlierBuildStep<TarPackageCreationStep>(this);
    if (!pStep) {
        if (error)
            *error = tr("No tarball creation step found.");
        return false;
    }
    m_deployService->setPackageFilePath(pStep->packageFilePath());
    return m_deployService->isDeploymentPossible(error);
}

BuildStepConfigWidget *UploadAndInstallTarPackageStep::createConfigWidget()
{
    return new SimpleBuildStepConfigWidget(this);
}

Core::Id UploadAndInstallTarPackageStep::stepId()
{
    return Core::Id("MaemoUploadAndInstallTarPackageStep");
}

QString UploadAndInstallTarPackageStep::displayName()
{
    return tr("Deploy tarball via SFTP upload");
}

} //namespace RemoteLinux
