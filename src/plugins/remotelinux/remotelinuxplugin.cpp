// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxplugin.h"

#include "customcommanddeploystep.h"
#include "genericdeploystep.h"
#include "genericdirectuploadstep.h"
#include "killappstep.h"
#include "linuxdevice.h"
#include "makeinstallstep.h"
#include "remotelinux_constants.h"
#include "remotelinuxcustomrunconfiguration.h"
#include "remotelinuxdebugsupport.h"
#include "remotelinuxrunconfiguration.h"
#include "remotelinuxtr.h"
#include "tarpackagecreationstep.h"
#include "tarpackagedeploystep.h"

#ifdef WITH_TESTS
#include "filesystemaccess_test.h"
#endif

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/fsengine/fsengine.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

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
        setPostRestore([needsMakeInstall](DeployConfiguration *dc, const Store &map) {
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

class RemoteLinuxPluginPrivate
{
public:
    LinuxDeviceFactory linuxDeviceFactory;
    RemoteLinuxRunConfigurationFactory runConfigurationFactory;
    RemoteLinuxCustomRunConfigurationFactory customRunConfigurationFactory;
    RemoteLinuxDeployConfigurationFactory deployConfigurationFactory;
    TarPackageCreationStepFactory tarPackageCreationStepFactory;
    TarPackageDeployStepFactory tarPackageDeployStepFactory;
    RemoteLinuxDeployStepFactory<GenericDirectUploadStepFactory> genericDirectUploadStepFactory;
    RemoteLinuxDeployStepFactory<GenericDeployStepFactory> rsyncDeployStepFactory;
    CustomCommandDeployStepFactory customCommandDeployStepFactory;
    KillAppStepFactory killAppStepFactory;
    RemoteLinuxDeployStepFactory<MakeInstallStepFactory> makeInstallStepFactory;
    RemoteLinuxRunWorkerFactory runWorkerFactory;
    RemoteLinuxDebugWorkerFactory debugWorkerFactory;
    RemoteLinuxQmlToolingWorkerFactory qmlToolingWorkerFactory;
};

static RemoteLinuxPluginPrivate *dd = nullptr;

RemoteLinuxPlugin::RemoteLinuxPlugin()
{
    setObjectName(QLatin1String("RemoteLinuxPlugin"));
    FSEngine::registerDeviceScheme(u"ssh");
}

RemoteLinuxPlugin::~RemoteLinuxPlugin()
{
    FSEngine::unregisterDeviceScheme(u"ssh");
    delete dd;
}

void RemoteLinuxPlugin::initialize()
{
    dd = new RemoteLinuxPluginPrivate;

#ifdef WITH_TESTS
    addTest<FileSystemAccessTest>();
#endif
}

} // namespace Internal
} // namespace RemoteLinux
