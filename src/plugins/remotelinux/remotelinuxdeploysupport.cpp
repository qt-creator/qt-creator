// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxdebugsupport.h"

#include "genericdeploystep.h"
#include "genericdirectuploadstep.h"
#include "makeinstallstep.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/store.h>

using namespace ProjectExplorer;

namespace RemoteLinux::Internal {

template <class Factory>
class RemoteLinuxDeployStepFactory : public Factory
{
public:
    RemoteLinuxDeployStepFactory()
    {
        Factory::setSupportedConfiguration(RemoteLinux::Constants::DeployToGenericLinux);
        Factory::setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
    }
};

class RemoteLinuxDeployConfigurationFactory : public DeployConfigurationFactory
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

        addInitialStep(Constants::MakeInstallStepId, needsMakeInstall);
        addInitialStep(Constants::KillAppStepId);

        // TODO: Rename RsyncDeployStep to something more generic.
        addInitialStep(Constants::GenericDeployStepId);
    }
};

void setupRemoteLinuxDeploySupport()
{
    static RemoteLinuxDeployConfigurationFactory deployConfigurationFactory;
    static RemoteLinuxDeployStepFactory<GenericDirectUploadStepFactory> genericDirectUploadStep;
    static RemoteLinuxDeployStepFactory<GenericDeployStepFactory> rsyncDeployStepFactory;
    static RemoteLinuxDeployStepFactory<MakeInstallStepFactory> makeInstallStepFactory;
}

} // RemoteLinux::Internal
