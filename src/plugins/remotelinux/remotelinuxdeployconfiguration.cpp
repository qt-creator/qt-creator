// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxdeployconfiguration.h"

#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/devicesupport/filetransferinterface.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
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
            dc->stepList()->insertStep(0, Constants::MakeInstallStepId);
        }
    });

    addInitialStep(Constants::MakeInstallStepId, needsMakeInstall);
    addInitialStep(Constants::KillAppStepId);

    // TODO: Rename RsyncDeployStep to something more generic.
    addInitialStep(Constants::RsyncDeployStepId);
}

} // RemoteLinux::Internal
