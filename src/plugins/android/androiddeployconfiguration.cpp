/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt-project.org/legal
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

#include "androiddeployconfiguration.h"
#include "androidconstants.h"
#include "androiddeploystep.h"
#include "androidpackageinstallationstep.h"
#include "androidpackagecreationstep.h"
#include "androiddeployqtstep.h"
#include "androidmanager.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qmakeprojectmanager/qmakeproject.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

using namespace ProjectExplorer;

namespace Android {
namespace Internal {

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

    if (id == ANDROID_DEPLOYCONFIGURATION_ID) {
        dc->stepList()->insertStep(0, new AndroidPackageInstallationStep(AndroidPackageInstallationStep::ProjectDirectory, dc->stepList()));
        dc->stepList()->insertStep(1, new AndroidPackageCreationStep(dc->stepList()));
        dc->stepList()->insertStep(2, new AndroidDeployStep(dc->stepList()));
    } else {
        dc->stepList()->insertStep(0, new AndroidPackageInstallationStep(AndroidPackageInstallationStep::BuildDirectory, dc->stepList()));
        dc->stepList()->insertStep(1, new AndroidDeployQtStep(dc->stepList()));
    }
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
    if (!qobject_cast<QmakeProjectManager::QmakeProject *>(parent->project()))
        return ids;

    if (!parent->project()->supportsKit(parent->kit()))
        return ids;

    ToolChain *tc = ToolChainKitInformation::toolChain(parent->kit());

    if (!tc || tc->targetAbi().osFlavor() != Abi::AndroidLinuxFlavor)
        return ids;

    QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(parent->kit());
    if (qt->type() != QLatin1String(Constants::ANDROIDQT))
        return ids;
    if (qt->qtVersion() < QtSupport::QtVersionNumber(5, 2, 0))
        ids << Core::Id(ANDROID_DEPLOYCONFIGURATION_ID);
    else
        ids << Core::Id(ANDROID_DEPLOYCONFIGURATION2_ID);
    return ids;
}

QString AndroidDeployConfigurationFactory::displayNameForId(Core::Id id) const
{
    if (id.name().startsWith(ANDROID_DC_PREFIX)
            || id.name().startsWith(ANDROID_DC2_PREFIX))
        return tr("Deploy on Android");
    return QString();
}

bool AndroidDeployConfigurationFactory::canHandle(Target *parent) const
{
    return AndroidManager::supportsAndroid(parent);
}

} // namespace Internal
} // namespace Android
