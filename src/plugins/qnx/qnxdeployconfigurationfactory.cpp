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

#include "qnxdeployconfigurationfactory.h"

#include "qnxconstants.h"
#include "qnxdeployconfiguration.h"
#include "qnxdevicefactory.h"

#include <projectexplorer/devicesupport/devicecheckbuildstep.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <remotelinux/genericdirectuploadstep.h>

namespace Qnx {
namespace Internal {

QnxDeployConfigurationFactory::QnxDeployConfigurationFactory(QObject *parent)
    : ProjectExplorer::DeployConfigurationFactory(parent)
{
}

QList<Core::Id> QnxDeployConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    QList<Core::Id> ids;

    if (canHandle(parent))
        ids << Core::Id(Constants::QNX_QNX_DEPLOYCONFIGURATION_ID);

    return ids;
}

QString QnxDeployConfigurationFactory::displayNameForId(Core::Id id) const
{
    if (id.name().startsWith(Constants::QNX_QNX_DEPLOYCONFIGURATION_ID))
        return tr("Deploy to QNX Device");

    return QString();
}

bool QnxDeployConfigurationFactory::canCreate(ProjectExplorer::Target *parent, Core::Id id) const
{
    return canHandle(parent) && id.name().startsWith(Constants::QNX_QNX_DEPLOYCONFIGURATION_ID);
}

ProjectExplorer::DeployConfiguration *QnxDeployConfigurationFactory::create(ProjectExplorer::Target *parent, Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;

    ProjectExplorer::DeployConfiguration * const dc = new QnxDeployConfiguration(parent, id,
        displayNameForId(id));
    dc->stepList()->insertStep(0, new ProjectExplorer::DeviceCheckBuildStep(dc->stepList(),
        ProjectExplorer::DeviceCheckBuildStep::stepId()));
    dc->stepList()->insertStep(1, new RemoteLinux::GenericDirectUploadStep(dc->stepList(),
        RemoteLinux::GenericDirectUploadStep::stepId()));
    return dc;
}

bool QnxDeployConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::DeployConfiguration *QnxDeployConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Core::Id id = ProjectExplorer::idFromMap(map);
    QnxDeployConfiguration * const dc = new QnxDeployConfiguration(parent, id, displayNameForId(id));
    if (!dc->fromMap(map)) {
        delete dc;
        return 0;
    }
    return dc;
}

bool QnxDeployConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::DeployConfiguration *QnxDeployConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    return new QnxDeployConfiguration(parent, qobject_cast<QnxDeployConfiguration *>(source));
}

bool QnxDeployConfigurationFactory::canHandle(ProjectExplorer::Target *t) const
{
    Core::Id deviceType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(t->kit());
    if (deviceType != QnxDeviceFactory::deviceType())
        return false;

    return true;
}

} // namespace Internal
} // namespace Qnx
