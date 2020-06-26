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

#include "remotelinuxdeployconfiguration.h"

#include "genericdirectuploadstep.h"
#include "makeinstallstep.h"
#include "remotelinuxcheckforfreediskspacestep.h"
#include "remotelinuxkillappstep.h"
#include "remotelinux_constants.h"
#include "rsyncdeploystep.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <QCoreApplication>

using namespace ProjectExplorer;

namespace RemoteLinux {

using namespace Internal;

Utils::Id genericDeployConfigurationId()
{
    return "DeployToGenericLinux";
}

namespace Internal {

RemoteLinuxDeployConfigurationFactory::RemoteLinuxDeployConfigurationFactory()
{
    setConfigBaseId(genericDeployConfigurationId());
    addSupportedTargetDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
    setDefaultDisplayName(QCoreApplication::translate("RemoteLinux",
                                                      "Deploy to Remote Linux Host"));
    setUseDeploymentDataView();

    const auto needsMakeInstall = [](Target *target)
    {
        const Project * const prj = target->project();
        return prj->deploymentKnowledge() == DeploymentKnowledge::Bad
                && prj->hasMakeInstallEquivalent();
    };
    setPostRestore([needsMakeInstall](DeployConfiguration *dc, const QVariantMap &map) {
        // 4.9 -> 4.10. See QTCREATORBUG-22689.
        if (map.value("_checkMakeInstall").toBool() && needsMakeInstall(dc->target())) {
            auto step = new MakeInstallStep(dc->stepList(), MakeInstallStep::stepId());
            dc->stepList()->insertStep(0, step);
        }
    });

    addInitialStep(MakeInstallStep::stepId(), needsMakeInstall);
    addInitialStep(RemoteLinuxCheckForFreeDiskSpaceStep::stepId());
    addInitialStep(RemoteLinuxKillAppStep::stepId());
    addInitialStep(RsyncDeployStep::stepId(), [](Target *target) {
        auto device = DeviceKitAspect::device(target->kit());
        return device && device->extraData(Constants::SupportsRSync).toBool();
    });
    addInitialStep(GenericDirectUploadStep::stepId(), [](Target *target) {
        auto device = DeviceKitAspect::device(target->kit());
        return device && !device->extraData(Constants::SupportsRSync).toBool();
    });
}

} // namespace Internal
} // namespace RemoteLinux
