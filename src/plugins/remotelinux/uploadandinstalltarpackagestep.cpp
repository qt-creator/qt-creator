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

#include "uploadandinstalltarpackagestep.h"

#include "remotelinuxdeployconfiguration.h"
#include "remotelinuxpackageinstaller.h"
#include "tarpackagecreationstep.h"

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

UploadAndInstallTarPackageService::UploadAndInstallTarPackageService()
    : d(new UploadAndInstallTarPackageServicePrivate)
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


UploadAndInstallTarPackageStep::UploadAndInstallTarPackageStep(BuildStepList *bsl, Utils::Id id)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    auto service = createDeployService<UploadAndInstallTarPackageService>();

    setDefaultDisplayName(displayName());
    setWidgetExpandedByDefault(false);

    setInternalInitializer([this, service] {
        const TarPackageCreationStep *pStep = nullptr;

        for (BuildStep *step : deployConfiguration()->stepList()->steps()) {
            if (step == this)
                break;
            if ((pStep = dynamic_cast<TarPackageCreationStep *>(step)))
                break;
        }
        if (!pStep)
            return CheckResult::failure(tr("No tarball creation step found."));

        service->setPackageFilePath(pStep->packageFilePath());
        return service->isDeploymentPossible();
    });
}

Utils::Id UploadAndInstallTarPackageStep::stepId()
{
    return "MaemoUploadAndInstallTarPackageStep";
}

QString UploadAndInstallTarPackageStep::displayName()
{
    return tr("Deploy tarball via SFTP upload");
}

} //namespace RemoteLinux
