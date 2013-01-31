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
#include "remotelinuxdeployconfigurationfactory.h"

#include "genericdirectuploadstep.h"
#include "remotelinuxcheckforfreediskspacestep.h"
#include "remotelinux_constants.h"
#include "remotelinuxdeployconfiguration.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
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

QList<Core::Id> RemoteLinuxDeployConfigurationFactory::availableCreationIds(Target *parent) const
{
    QList<Core::Id> ids;
    if (!parent->project()->supportsKit(parent->kit()))
        return ids;
    ProjectExplorer::ToolChain *tc
            = ProjectExplorer::ToolChainKitInformation::toolChain(parent->kit());
    if (!tc || tc->targetAbi().os() != ProjectExplorer::Abi::LinuxOS)
        return ids;
    const Core::Id devType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(parent->kit());
    if (devType == Constants::GenericLinuxOsType)
        ids << genericDeployConfigurationId();
    return ids;
}

QString RemoteLinuxDeployConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id == genericDeployConfigurationId())
        return genericLinuxDisplayName();
    return QString();
}

bool RemoteLinuxDeployConfigurationFactory::canCreate(Target *parent, const Core::Id id) const
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
    return Core::Id("DeployToGenericLinux");
}

} // namespace Internal
} // namespace RemoteLinux
