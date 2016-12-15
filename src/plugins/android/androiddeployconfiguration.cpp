/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androiddeployconfiguration.h"
#include "androidconstants.h"
#include "androiddeployqtstep.h"
#include "androidmanager.h"
#include "androidqtsupport.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

using namespace ProjectExplorer;

namespace Android {
namespace Internal {

// Qt 5.2 has a new form of deployment
const char ANDROID_DEPLOYCONFIGURATION_ID[] = "Qt4ProjectManager.AndroidDeployConfiguration2";

AndroidDeployConfiguration::AndroidDeployConfiguration(Target *parent, Core::Id id)
    : DeployConfiguration(parent, id)
{
    setDisplayName(tr("Deploy to Android device"));
    setDefaultDisplayName(displayName());
}

AndroidDeployConfiguration::AndroidDeployConfiguration(Target *parent, DeployConfiguration *source)
    : DeployConfiguration(parent, source)
{
    cloneSteps(source);
}

AndroidDeployConfigurationFactory::AndroidDeployConfigurationFactory(QObject *parent)
    : DeployConfigurationFactory(parent)
{
    setObjectName(QLatin1String("AndroidDeployConfigurationFactory"));
}

bool AndroidDeployConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

DeployConfiguration *AndroidDeployConfigurationFactory::create(Target *parent, Core::Id id)
{
    AndroidDeployConfiguration *dc = new AndroidDeployConfiguration(parent, id);
    dc->stepList()->insertStep(0, new AndroidDeployQtStep(dc->stepList()));
    return dc;
}

bool AndroidDeployConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

DeployConfiguration *AndroidDeployConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    AndroidDeployConfiguration *dc = new AndroidDeployConfiguration(parent, idFromMap(map));
    if (dc->fromMap(map))
        return dc;

    delete dc;
    return 0;
}

bool AndroidDeployConfigurationFactory::canClone(Target *parent, DeployConfiguration *source) const
{
    if (!AndroidManager::supportsAndroid(parent))
        return false;
    return canCreate(parent, source->id());
}

DeployConfiguration *AndroidDeployConfigurationFactory::clone(Target *parent, DeployConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    return new AndroidDeployConfiguration(parent, source);
}

QList<Core::Id> AndroidDeployConfigurationFactory::availableCreationIds(Target *parent) const
{
    QList<Core::Id> ids;
    if (!parent->project()->supportsKit(parent->kit()))
        return ids;

    ToolChain *tc = ToolChainKitInformation::toolChain(parent->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    if (!tc || tc->targetAbi().osFlavor() != Abi::AndroidLinuxFlavor)
        return ids;

    QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(parent->kit());
    if (!qt || qt->type() != QLatin1String(Constants::ANDROIDQT))
        return ids;
    ids << Core::Id(ANDROID_DEPLOYCONFIGURATION_ID);
    return ids;
}

QString AndroidDeployConfigurationFactory::displayNameForId(Core::Id id) const
{
    if (id == Core::Id(ANDROID_DEPLOYCONFIGURATION_ID))
        return tr("Deploy on Android");
    return QString();
}

} // namespace Internal
} // namespace Android
