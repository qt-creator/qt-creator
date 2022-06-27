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

using namespace ProjectExplorer;

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
    GenericDeployStepFactory<TarPackageCreationStep> tarPackageCreationStepFactory;
    TarPackageDeployStepFactory tarPackageDeployStepFactory;
    GenericDeployStepFactory<GenericDirectUploadStep> genericDirectUploadStepFactory;
    GenericDeployStepFactory<RsyncDeployStep> rsyncDeployStepFactory;
    CustomCommandDeployStepFactory customCommandDeployStepFactory;
    GenericDeployStepFactory<KillAppStep> killAppStepFactory;
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
}

RemoteLinuxPlugin::~RemoteLinuxPlugin()
{
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
