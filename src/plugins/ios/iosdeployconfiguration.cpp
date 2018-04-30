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

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/target.h>

#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

using namespace ProjectExplorer;

namespace Ios {
namespace Internal {

IosDeployConfiguration::IosDeployConfiguration(Target *parent, Core::Id id)
    : DeployConfiguration(parent, id)
{
}

void IosDeployConfiguration::initialize()
{
    stepList()->appendStep(new IosDeployStep(stepList()));
}

IosDeployConfigurationFactory::IosDeployConfigurationFactory()
{
    registerDeployConfiguration<IosDeployConfiguration>("Qt4ProjectManager.IosDeployConfiguration");
    setSupportedProjectType(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
    addSupportedTargetDeviceType(Constants::IOS_DEVICE_TYPE);
    addSupportedTargetDeviceType(Constants::IOS_SIMULATOR_TYPE);
    setDefaultDisplayName(IosDeployConfiguration::tr("Deploy on iOS"));
}

} // namespace Internal
} // namespace Ios
