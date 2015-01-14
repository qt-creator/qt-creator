/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#include "qnxdeploystepfactory.h"
#include "qnxconstants.h"
#include "qnxdeviceconfigurationfactory.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/devicesupport/devicecheckbuildstep.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <remotelinux/genericdirectuploadstep.h>

using namespace Qnx;
using namespace Qnx::Internal;

QnxDeployStepFactory::QnxDeployStepFactory()
    : ProjectExplorer::IBuildStepFactory()
{
}

QList<Core::Id> QnxDeployStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        return QList<Core::Id>();

    Core::Id deviceType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(parent->target()->kit());
    if (deviceType != QnxDeviceConfigurationFactory::deviceType())
        return QList<Core::Id>();

    return QList<Core::Id>() << RemoteLinux::GenericDirectUploadStep::stepId()
                             << ProjectExplorer::DeviceCheckBuildStep::stepId();
}

QString QnxDeployStepFactory::displayNameForId(Core::Id id) const
{
    if (id == RemoteLinux::GenericDirectUploadStep::stepId())
        return RemoteLinux::GenericDirectUploadStep::displayName();
    else if (id == ProjectExplorer::DeviceCheckBuildStep::stepId())
        return ProjectExplorer::DeviceCheckBuildStep::stepDisplayName();

    return QString();
}

bool QnxDeployStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

ProjectExplorer::BuildStep *QnxDeployStepFactory::create(ProjectExplorer::BuildStepList *parent, Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;

    if (id == RemoteLinux::GenericDirectUploadStep::stepId())
        return new RemoteLinux::GenericDirectUploadStep(parent, id);
    else if (id == ProjectExplorer::DeviceCheckBuildStep::stepId())
        return new ProjectExplorer::DeviceCheckBuildStep(parent, id);
    return 0;
}

bool QnxDeployStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep *QnxDeployStepFactory::restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    ProjectExplorer::BuildStep * const bs = create(parent, ProjectExplorer::idFromMap(map));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

bool QnxDeployStepFactory::canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const
{
    return canCreate(parent, product->id());
}

ProjectExplorer::BuildStep *QnxDeployStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product)
{
    if (!canClone(parent, product))
        return 0;

    if (RemoteLinux::GenericDirectUploadStep * const other = qobject_cast<RemoteLinux::GenericDirectUploadStep*>(product))
        return new RemoteLinux::GenericDirectUploadStep(parent, other);
    else if (ProjectExplorer::DeviceCheckBuildStep * const other = qobject_cast<ProjectExplorer::DeviceCheckBuildStep*>(product))
        return new ProjectExplorer::DeviceCheckBuildStep(parent, other);

    return 0;
}
