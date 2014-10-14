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
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberrydeployconfigurationfactory.h"

#include "qnxconstants.h"
#include "blackberrycheckdevicestatusstep.h"
#include "blackberrydeployconfiguration.h"
#include "blackberrycreatepackagestep.h"
#include "blackberrydeploystep.h"
#include "blackberrydeviceconfigurationfactory.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <remotelinux/genericdirectuploadstep.h>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryDeployConfigurationFactory::BlackBerryDeployConfigurationFactory(QObject *parent) :
    ProjectExplorer::DeployConfigurationFactory(parent)
{
}

BlackBerryDeployConfigurationFactory::~BlackBerryDeployConfigurationFactory()
{
}

QList<Core::Id> BlackBerryDeployConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    QList<Core::Id> result;

    QmakeProjectManager::QmakeProject *project = qobject_cast<QmakeProjectManager::QmakeProject *>(parent->project());
    if (!project)
        return result;

    Core::Id deviceType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(parent->kit());
    if (deviceType != BlackBerryDeviceConfigurationFactory::deviceType())
        return result;

    result << Core::Id(Constants::QNX_BB_DEPLOYCONFIGURATION_ID);
    return result;
}

QString BlackBerryDeployConfigurationFactory::displayNameForId(Core::Id id) const
{
    if (id == Constants::QNX_BB_DEPLOYCONFIGURATION_ID)
        return tr("Deploy to BlackBerry Device");

    return QString();
}

bool BlackBerryDeployConfigurationFactory::canCreate(ProjectExplorer::Target *parent,
                                              const Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

ProjectExplorer::DeployConfiguration *BlackBerryDeployConfigurationFactory::create(
        ProjectExplorer::Target *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;

    BlackBerryDeployConfiguration *dc = new BlackBerryDeployConfiguration(parent);
    dc->stepList()->insertStep(0, new BlackBerryCheckDeviceStatusStep(dc->stepList()));
    dc->stepList()->insertStep(1, new BlackBerryCreatePackageStep(dc->stepList()));
    dc->stepList()->insertStep(2, new BlackBerryDeployStep(dc->stepList()));
    return dc;
}

bool BlackBerryDeployConfigurationFactory::canRestore(ProjectExplorer::Target *parent,
                                               const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::DeployConfiguration *BlackBerryDeployConfigurationFactory::restore(
        ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    BlackBerryDeployConfiguration *dc = new BlackBerryDeployConfiguration(parent);
    if (dc->fromMap(map))
        return dc;

    delete dc;
    return 0;
}

bool BlackBerryDeployConfigurationFactory::canClone(ProjectExplorer::Target *parent,
                                             ProjectExplorer::DeployConfiguration *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::DeployConfiguration *BlackBerryDeployConfigurationFactory::clone(
        ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    BlackBerryDeployConfiguration *old = static_cast<BlackBerryDeployConfiguration *>(source);
    return new BlackBerryDeployConfiguration(parent, old);
}
