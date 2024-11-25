// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxdeploysupport.h"

#include "genericdeploystep.h"
#include "genericdirectuploadstep.h"
#include "makeinstallstep.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;

namespace RemoteLinux::Internal {

class RemoteLinuxDeployConfigurationFactory final : public DeployConfigurationFactory
{
public:
    RemoteLinuxDeployConfigurationFactory()
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
        setPostRestore([needsMakeInstall](DeployConfiguration *dc, const Utils::Store &map) {
            // 4.9 -> 4.10. See QTCREATORBUG-22689.
            if (map.value("_checkMakeInstall").toBool() && needsMakeInstall(dc->target())) {
                dc->stepList()->insertStep(0, Constants::MakeInstallStepId);
            }
        });

        // Make sure we can use the steps below.
        setupGenericDirectUploadStep();
        setupGenericDeployStep();
        setupMakeInstallStep();

        addInitialStep(Constants::MakeInstallStepId, needsMakeInstall);
        addInitialStep(Constants::KillAppStepId);
        addInitialStep(Constants::GenericDeployStepId);
    }
};

void setupRemoteLinuxDeploySupport()
{
    static RemoteLinuxDeployConfigurationFactory deployConfigurationFactory;
}

} // RemoteLinux::Internal
