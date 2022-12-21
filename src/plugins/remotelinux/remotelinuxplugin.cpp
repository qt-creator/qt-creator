// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotelinuxplugin.h"

#include "customcommanddeploystep.h"
#include "genericdirectuploadstep.h"
#include "killappstep.h"
#include "linuxdevice.h"
#include "makeinstallstep.h"
#include "remotelinux_constants.h"
#include "remotelinuxdeployconfiguration.h"
#include "remotelinuxqmltoolingsupport.h"
#include "remotelinuxcustomrunconfiguration.h"
#include "remotelinuxdebugsupport.h"
#include "remotelinuxdeployconfiguration.h"
#include "remotelinuxrunconfiguration.h"
#include "rsyncdeploystep.h"
#include "tarpackagecreationstep.h"
#include "tarpackagedeploystep.h"

#ifdef WITH_TESTS
#include "filesystemaccess_test.h"
#endif

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/fsengine/fsengine.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {

template <class Step>
class GenericDeployStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    GenericDeployStepFactory()
    {
        registerStep<Step>(Step::stepId());
        setDisplayName(Step::displayName());
        setSupportedConfiguration(RemoteLinux::Constants::DeployToGenericLinux);
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
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
    GenericDeployStepFactory<GenericDirectUploadStep> genericDirectUploadStepFactory;
    GenericDeployStepFactory<RsyncDeployStep> rsyncDeployStepFactory;
    CustomCommandDeployStepFactory customCommandDeployStepFactory;
    KillAppStepFactory killAppStepFactory;
    GenericDeployStepFactory<MakeInstallStep> makeInstallStepFactory;

    const QList<Utils::Id> supportedRunConfigs {
        runConfigurationFactory.runConfigurationId(),
        customRunConfigurationFactory.runConfigurationId(),
        "QmlProjectManager.QmlRunConfiguration"
    };

    RunWorkerFactory runnerFactory{
        RunWorkerFactory::make<SimpleTargetRunner>(),
        {ProjectExplorer::Constants::NORMAL_RUN_MODE},
        supportedRunConfigs,
        {Constants::GenericLinuxOsType}
    };
    RunWorkerFactory debuggerFactory{
        RunWorkerFactory::make<LinuxDeviceDebugSupport>(),
        {ProjectExplorer::Constants::DEBUG_RUN_MODE},
        supportedRunConfigs,
        {Constants::GenericLinuxOsType}
    };
    RunWorkerFactory qmlToolingFactory{
        RunWorkerFactory::make<RemoteLinuxQmlToolingSupport>(),
        {ProjectExplorer::Constants::QML_PROFILER_RUN_MODE,
         ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE},
        supportedRunConfigs,
        {Constants::GenericLinuxOsType}
    };
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

QVector<QObject *> RemoteLinuxPlugin::createTestObjects() const
{
    return {
#ifdef WITH_TESTS
        new FileSystemAccessTest,
#endif
    };
}

bool RemoteLinuxPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    dd = new RemoteLinuxPluginPrivate;

    return true;
}

} // namespace Internal
} // namespace RemoteLinux
