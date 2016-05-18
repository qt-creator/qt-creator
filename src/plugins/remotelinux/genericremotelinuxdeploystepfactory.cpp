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

#include "genericremotelinuxdeploystepfactory.h"

#include "genericdirectuploadstep.h"
#include "remotelinuxcheckforfreediskspacestep.h"
#include "remotelinuxdeployconfiguration.h"
#include "remotelinuxcustomcommanddeploymentstep.h"
#include "tarpackagecreationstep.h"
#include "uploadandinstalltarpackagestep.h"

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

GenericRemoteLinuxDeployStepFactory::GenericRemoteLinuxDeployStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QList<BuildStepInfo> GenericRemoteLinuxDeployStepFactory::availableSteps(BuildStepList *parent) const
{
    if (!qobject_cast<RemoteLinuxDeployConfiguration *>(parent->parent()))
        return {};

    return {{ TarPackageCreationStep::stepId(),
              TarPackageCreationStep::displayName() },
            { UploadAndInstallTarPackageStep::stepId(),
              UploadAndInstallTarPackageStep::displayName() },
            { GenericDirectUploadStep::stepId(),
              GenericDirectUploadStep::displayName() },
            { GenericRemoteLinuxCustomCommandDeploymentStep::stepId(),
              GenericRemoteLinuxCustomCommandDeploymentStep::stepDisplayName() },
            { RemoteLinuxCheckForFreeDiskSpaceStep::stepId(),
              RemoteLinuxCheckForFreeDiskSpaceStep::stepDisplayName() }};
}

BuildStep *GenericRemoteLinuxDeployStepFactory::create(BuildStepList *parent, Core::Id id)
{
    if (id == TarPackageCreationStep::stepId())
        return new TarPackageCreationStep(parent);
    if (id == UploadAndInstallTarPackageStep::stepId())
        return new UploadAndInstallTarPackageStep(parent);
    if (id == GenericDirectUploadStep::stepId())
        return new GenericDirectUploadStep(parent, GenericDirectUploadStep::stepId());
    if (id == GenericRemoteLinuxCustomCommandDeploymentStep::stepId())
        return new GenericRemoteLinuxCustomCommandDeploymentStep(parent);
    if (id == RemoteLinuxCheckForFreeDiskSpaceStep::stepId())
        return new RemoteLinuxCheckForFreeDiskSpaceStep(parent);
    return 0;
}

BuildStep *GenericRemoteLinuxDeployStepFactory::clone(BuildStepList *parent, BuildStep *product)
{
    if (TarPackageCreationStep * const other = qobject_cast<TarPackageCreationStep *>(product))
        return new TarPackageCreationStep(parent, other);
    if (UploadAndInstallTarPackageStep * const other = qobject_cast<UploadAndInstallTarPackageStep*>(product))
        return new UploadAndInstallTarPackageStep(parent, other);
    if (GenericDirectUploadStep * const other = qobject_cast<GenericDirectUploadStep *>(product))
        return new GenericDirectUploadStep(parent, other);
    if (GenericRemoteLinuxCustomCommandDeploymentStep * const other = qobject_cast<GenericRemoteLinuxCustomCommandDeploymentStep *>(product))
        return new GenericRemoteLinuxCustomCommandDeploymentStep(parent, other);
    if (RemoteLinuxCheckForFreeDiskSpaceStep * const other = qobject_cast<RemoteLinuxCheckForFreeDiskSpaceStep *>(product))
        return new RemoteLinuxCheckForFreeDiskSpaceStep(parent, other);
    return 0;
}

} // namespace Internal
} // namespace RemoteLinux
