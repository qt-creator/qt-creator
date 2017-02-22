/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnxdeploystepfactory.h"
#include "qnxconstants.h"
#include "qnxdevicefactory.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/devicesupport/devicecheckbuildstep.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <remotelinux/genericdirectuploadstep.h>

using namespace ProjectExplorer;

namespace Qnx {
namespace Internal {

QnxDeployStepFactory::QnxDeployStepFactory()
    : ProjectExplorer::IBuildStepFactory()
{
}

QList<BuildStepInfo> QnxDeployStepFactory::availableSteps(BuildStepList *parent) const
{
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        return {};

    Core::Id deviceType = DeviceTypeKitInformation::deviceTypeId(parent->target()->kit());
    if (deviceType != QnxDeviceFactory::deviceType())
        return {};

    return {{RemoteLinux::GenericDirectUploadStep::stepId(),
             RemoteLinux::GenericDirectUploadStep::displayName()},
            {DeviceCheckBuildStep::stepId(),
             DeviceCheckBuildStep::stepDisplayName()}};
}

ProjectExplorer::BuildStep *QnxDeployStepFactory::create(ProjectExplorer::BuildStepList *parent, Core::Id id)
{
    if (id == RemoteLinux::GenericDirectUploadStep::stepId())
        return new RemoteLinux::GenericDirectUploadStep(parent, id);
    else if (id == ProjectExplorer::DeviceCheckBuildStep::stepId())
        return new ProjectExplorer::DeviceCheckBuildStep(parent, id);
    return 0;
}

ProjectExplorer::BuildStep *QnxDeployStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product)
{
    if (RemoteLinux::GenericDirectUploadStep * const other = qobject_cast<RemoteLinux::GenericDirectUploadStep*>(product))
        return new RemoteLinux::GenericDirectUploadStep(parent, other);
    else if (ProjectExplorer::DeviceCheckBuildStep * const other = qobject_cast<ProjectExplorer::DeviceCheckBuildStep*>(product))
        return new ProjectExplorer::DeviceCheckBuildStep(parent, other);

    return 0;
}

} // namespace Internal
} // namespace Qnx
