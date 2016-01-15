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

#include "remotelinuxdeployconfigurationfactory.h"

#include "genericdirectuploadstep.h"
#include "remotelinuxcheckforfreediskspacestep.h"
#include "remotelinux_constants.h"
#include "remotelinuxdeployconfiguration.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <QCoreApplication>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {
namespace {
QString genericLinuxDisplayName() {
    return QCoreApplication::translate("RemoteLinux", "Deploy to Remote Linux Host");
}
} // anonymous namespace

RemoteLinuxDeployConfigurationFactory::RemoteLinuxDeployConfigurationFactory(QObject *parent)
    : DeployConfigurationFactory(parent)
{ setObjectName(QLatin1String("RemoteLinuxDeployConfiguration"));}

QList<Core::Id> RemoteLinuxDeployConfigurationFactory::availableCreationIds(Target *target) const
{
    QList<Core::Id> ids;
    if (!target->project()->supportsKit(target->kit()))
        return ids;

    const Core::Id deviceType = DeviceTypeKitInformation::deviceTypeId(target->kit());
    if (deviceType != RemoteLinux::Constants::GenericLinuxOsType)
        return ids;

    ids << genericDeployConfigurationId();
    return ids;
}

QString RemoteLinuxDeployConfigurationFactory::displayNameForId(Core::Id id) const
{
    if (id == genericDeployConfigurationId())
        return genericLinuxDisplayName();
    return QString();
}

bool RemoteLinuxDeployConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

DeployConfiguration *RemoteLinuxDeployConfigurationFactory::create(Target *parent,
    const Core::Id id)
{
    Q_ASSERT(canCreate(parent, id));

    DeployConfiguration * const dc = new RemoteLinuxDeployConfiguration(parent, id,
        genericLinuxDisplayName());
    dc->stepList()->insertStep(0, new RemoteLinuxCheckForFreeDiskSpaceStep(dc->stepList()));
    dc->stepList()->insertStep(1, new GenericDirectUploadStep(dc->stepList(),
        GenericDirectUploadStep::stepId()));
    return dc;
}

bool RemoteLinuxDeployConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

DeployConfiguration *RemoteLinuxDeployConfigurationFactory::restore(Target *parent,
    const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Core::Id id = idFromMap(map);
    RemoteLinuxDeployConfiguration * const dc = new RemoteLinuxDeployConfiguration(parent, id,
        genericLinuxDisplayName());
    if (!dc->fromMap(map)) {
        delete dc;
        return 0;
    }
    return dc;
}

bool RemoteLinuxDeployConfigurationFactory::canClone(Target *parent, DeployConfiguration *product) const
{
    return canCreate(parent, product->id());
}

DeployConfiguration *RemoteLinuxDeployConfigurationFactory::clone(Target *parent,
    DeployConfiguration *product)
{
    if (!canClone(parent, product))
        return 0;
    return new RemoteLinuxDeployConfiguration(parent,
        qobject_cast<RemoteLinuxDeployConfiguration *>(product));
}

Core::Id RemoteLinuxDeployConfigurationFactory::genericDeployConfigurationId()
{
    return "DeployToGenericLinux";
}

} // namespace Internal
} // namespace RemoteLinux
