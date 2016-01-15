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

#include "iosconstants.h"
#include "iosdeploystep.h"
#include "iosdeployconfiguration.h"
#include "iosmanager.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/target.h>

#include <qmakeprojectmanager/qmakeproject.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

using namespace ProjectExplorer;

namespace Ios {
namespace Internal {

const char IOS_DEPLOYCONFIGURATION_ID[] = "Qt4ProjectManager.IosDeployConfiguration";
const char IOS_DC_PREFIX[] = "Qt4ProjectManager.IosDeployConfiguration.";

IosDeployConfiguration::IosDeployConfiguration(Target *parent, Core::Id id)
    : DeployConfiguration(parent, id)
{
    setDisplayName(tr("Deploy to iOS"));
    setDefaultDisplayName(displayName());
}

IosDeployConfiguration::IosDeployConfiguration(Target *parent, DeployConfiguration *source)
    : DeployConfiguration(parent, source)
{
    cloneSteps(source);
}

IosDeployConfigurationFactory::IosDeployConfigurationFactory(QObject *parent)
    : DeployConfigurationFactory(parent)
{
    setObjectName(QLatin1String("IosDeployConfigurationFactory"));
}

bool IosDeployConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

DeployConfiguration *IosDeployConfigurationFactory::create(Target *parent, Core::Id id)
{
    IosDeployConfiguration *dc = new IosDeployConfiguration(parent, id);
    dc->stepList()->insertStep(0, new IosDeployStep(dc->stepList()));
    return dc;
}

bool IosDeployConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

DeployConfiguration *IosDeployConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    IosDeployConfiguration *dc = new IosDeployConfiguration(parent, idFromMap(map));
    if (dc->fromMap(map))
        return dc;

    delete dc;
    return 0;
}

bool IosDeployConfigurationFactory::canClone(Target *parent, DeployConfiguration *source) const
{
    if (!IosManager::supportsIos(parent))
        return false;
    return source->id() == IOS_DEPLOYCONFIGURATION_ID;
}

DeployConfiguration *IosDeployConfigurationFactory::clone(Target *parent, DeployConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    return new IosDeployConfiguration(parent, source);
}

QList<Core::Id> IosDeployConfigurationFactory::availableCreationIds(Target *parent) const
{
    QList<Core::Id> ids;
    if (!qobject_cast<QmakeProjectManager::QmakeProject *>(parent->project()))
        return ids;
    if (!parent->project()->supportsKit(parent->kit()))
        return ids;
    if (!IosManager::supportsIos(parent))
        return ids;
    ids << Core::Id(IOS_DEPLOYCONFIGURATION_ID);
    return ids;
}

QString IosDeployConfigurationFactory::displayNameForId(Core::Id id) const
{
    if (id.name().startsWith(IOS_DC_PREFIX))
        return tr("Deploy on iOS");
    return QString();
}

} // namespace Internal
} // namespace Ios
