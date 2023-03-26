// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxdeployconfiguration.h"

#include "makeinstallstep.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;

namespace RemoteLinux::Internal {

RemoteLinuxDeployConfigurationFactory::RemoteLinuxDeployConfigurationFactory()
{
    setConfigBaseId(RemoteLinux::Constants::DeployToGenericLinux);
    addSupportedTargetDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
    setDefaultDisplayName(Tr::tr("Deploy to Remote Linux Host"));
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

    addInitialStep(Constants::MakeInstallStepId, needsMakeInstall);
    addInitialStep(Constants::KillAppStepId);

    // Todo: Check: Instead of having two different steps here, have one
    // and shift the logic into the implementation there?
    addInitialStep(Constants::RsyncDeployStepId, [](Target *target) {
        auto runDevice = DeviceKitAspect::device(target->kit());
        auto buildDevice = BuildDeviceKitAspect::device(target->kit());
        if (runDevice == buildDevice)
            return false;
        // FIXME: That's not the full truth, we need support from the build
        // device, too.
        return runDevice && runDevice->extraData(Constants::SupportsRSync).toBool();
    });
    addInitialStep(Constants::DirectUploadStepId, [](Target *target) {
        auto runDevice = DeviceKitAspect::device(target->kit());
        auto buildDevice = BuildDeviceKitAspect::device(target->kit());
        if (runDevice == buildDevice)
            return true;
        return runDevice && !runDevice->extraData(Constants::SupportsRSync).toBool();
    });
}

} // RemoteLinux::Internal
