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

#include "iosplugin.h"

#include "iosbuildconfiguration.h"
#include "iosbuildstep.h"
#include "iosconfigurations.h"
#include "iosconstants.h"
#include "iosdeploystep.h"
#include "iosdevice.h"
#include "iosdsymbuildstep.h"
#include "iosqtversion.h"
#include "iosrunner.h"
#include "iossettingspage.h"
#include "iossimulator.h"
#include "iostoolhandler.h"
#include "iosrunconfiguration.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/runconfiguration.h>

#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

using namespace ProjectExplorer;
using namespace QtSupport;

namespace Ios {
namespace Internal {

Q_LOGGING_CATEGORY(iosLog, "qtc.ios.common", QtWarningMsg)

class IosDeployConfigurationFactory : public DeployConfigurationFactory
{
public:
    IosDeployConfigurationFactory()
    {
        setConfigBaseId("Qt4ProjectManager.IosDeployConfiguration");
        setSupportedProjectType(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
        addSupportedTargetDeviceType(Constants::IOS_DEVICE_TYPE);
        addSupportedTargetDeviceType(Constants::IOS_SIMULATOR_TYPE);
        setDefaultDisplayName(QCoreApplication::translate("Ios::Internal", "Deploy on iOS"));
        addInitialStep(IosDeployStepFactory::stepId());
    }
};

class IosPluginPrivate
{
public:
    IosBuildConfigurationFactory buildConfigurationFactory;
    IosToolChainFactory toolChainFactory;
    IosRunConfigurationFactory runConfigurationFactory;
    IosSettingsPage settingsPage;
    IosQtVersionFactory qtVersionFactory;
    IosDeviceFactory deviceFactory;
    IosSimulatorFactory simulatorFactory;
    IosBuildStepFactory buildStepFactory;
    IosDeployStepFactory deployStepFactory;
    IosDsymBuildStepFactory dsymBuildStepFactory;
    IosDeployConfigurationFactory deployConfigurationFactory;

    RunWorkerFactory runWorkerFactory{
        RunWorkerFactory::make<IosRunSupport>(),
        {ProjectExplorer::Constants::NORMAL_RUN_MODE},
        {runConfigurationFactory.runConfigurationId()}
    };
    RunWorkerFactory debugWorkerFactory{
        RunWorkerFactory::make<IosDebugSupport>(),
        {ProjectExplorer::Constants::DEBUG_RUN_MODE},
        {runConfigurationFactory.runConfigurationId()}
    };
    RunWorkerFactory qmlProfilerWorkerFactory{
        RunWorkerFactory::make<IosQmlProfilerSupport>(),
        {ProjectExplorer::Constants::QML_PROFILER_RUN_MODE},
        {runConfigurationFactory.runConfigurationId()}
    };
};

IosPlugin::~IosPlugin()
{
    delete d;
}

bool IosPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    qRegisterMetaType<Ios::IosToolHandler::Dict>("Ios::IosToolHandler::Dict");

    IosConfigurations::initialize();

    d = new IosPluginPrivate;

    return true;
}

} // namespace Internal
} // namespace Ios
