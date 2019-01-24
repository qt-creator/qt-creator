/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidplugin.h"

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androiddebugsupport.h"
#include "androiddeployqtstep.h"
#include "androiddevice.h"
#include "androiddevicefactory.h"
#include "androidgdbserverkitinformation.h"
#include "androidmanager.h"
#include "androidmanifesteditorfactory.h"
#include "androidpackageinstallationstep.h"
#include "androidpotentialkit.h"
#include "androidqmltoolingsupport.h"
#include "androidqtversionfactory.h"
#include "androidrunconfiguration.h"
#include "androidruncontrol.h"
#include "androidsettingspage.h"
#include "androidtoolchain.h"
#include "javaeditor.h"

#ifdef HAVE_QBS
#  include "androidqbspropertyprovider.h"
#endif

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtversionmanager.h>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Constants;

namespace Android {
namespace Internal {

class AndroidDeployConfigurationFactory : public DeployConfigurationFactory
{
public:
    AndroidDeployConfigurationFactory()
    {
        setConfigBaseId("Qt4ProjectManager.AndroidDeployConfiguration2");
        addSupportedTargetDeviceType(Constants::ANDROID_DEVICE_TYPE);
        setDefaultDisplayName(QCoreApplication::translate("Android::Internal",
                                                          "Deploy to Android device"));
        addInitialStep(AndroidDeployQtStep::stepId());
    }
};

class AndroidRunConfigurationFactory : public RunConfigurationFactory
{
public:
    AndroidRunConfigurationFactory()
    {
        registerRunConfiguration<Android::AndroidRunConfiguration>
                ("Qt4ProjectManager.AndroidRunConfiguration:");
        addSupportedTargetDeviceType(Android::Constants::ANDROID_DEVICE_TYPE);
        addRunWorkerFactory<AndroidRunSupport>(NORMAL_RUN_MODE);
        addRunWorkerFactory<AndroidDebugSupport>(DEBUG_RUN_MODE);
        addRunWorkerFactory<AndroidQmlToolingSupport>(QML_PROFILER_RUN_MODE);
        addRunWorkerFactory<AndroidQmlToolingSupport>(QML_PREVIEW_RUN_MODE);
    }
};

class AndroidPluginPrivate : public QObject
{
public:
    AndroidPluginPrivate()
    {
        connect(SessionManager::instance(), &SessionManager::projectAdded, this, [=](Project *project) {
            for (Target *target : project->targets())
                handleNewTarget(target);
            connect(project, &Project::addedTarget, this, &AndroidPluginPrivate::handleNewTarget);
        });
    }

    void handleNewTarget(Target *target)
    {
        if (DeviceTypeKitInformation::deviceTypeId(target->kit()) != Android::Constants::ANDROID_DEVICE_TYPE)
            return;

        for (BuildConfiguration *bc : target->buildConfigurations())
            handleNewBuildConfiguration(bc);

        connect(target, &Target::addedBuildConfiguration,
                this, &AndroidPluginPrivate::handleNewBuildConfiguration);
    }

    void handleNewBuildConfiguration(BuildConfiguration *bc)
    {
        connect(bc->target()->project(), &Project::parsingFinished, bc, [bc] {
            AndroidManager::updateGradleProperties(bc->target());
        });
    }

    AndroidConfigurations androidConfiguration;
    AndroidSettingsPage settingsPage;
    AndroidDeployQtStepFactory deployQtStepFactory;
    AndroidQtVersionFactory qtVersionFactory;
    AndroidToolChainFactory toolChainFactory;
    AndroidDeployConfigurationFactory deployConfigurationFactory;
    AndroidDeviceFactory deviceFactory;
    AndroidPotentialKit potentialKit;
    JavaEditorFactory javaEditorFactory;
    AndroidPackageInstallationFactory packackeInstallationFactory;
    AndroidManifestEditorFactory manifestEditorFactory;
    AndroidRunConfigurationFactory runConfigFactory;
    AndroidBuildApkStepFactory buildApkStepFactory;
};

AndroidPlugin::~AndroidPlugin()
{
    delete d;
}

bool AndroidPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);

    RunControl::registerWorker(QML_PREVIEW_RUN_MODE, [](RunControl *runControl) -> RunWorker* {
        const Runnable runnable = runControl->runConfiguration()->runnable();
        return new AndroidQmlToolingSupport(runControl, runnable.executable);
    }, [](RunConfiguration *runConfig) {
        return runConfig->isEnabled()
                && runConfig->id().name().startsWith("QmlProjectManager.QmlRunConfiguration")
                && DeviceTypeKitInformation::deviceTypeId(runConfig->target()->kit())
                    == Android::Constants::ANDROID_DEVICE_TYPE;
    });

    d = new AndroidPluginPrivate;

    KitManager::registerKitInformation<Internal::AndroidGdbServerKitInformation>();

    connect(KitManager::instance(), &KitManager::kitsLoaded,
            this, &AndroidPlugin::kitsRestored);

    return true;
}

void AndroidPlugin::kitsRestored()
{
    AndroidConfigurations::updateAutomaticKitList();
    connect(QtSupport::QtVersionManager::instance(), &QtSupport::QtVersionManager::qtVersionsChanged,
            AndroidConfigurations::instance(), &AndroidConfigurations::updateAutomaticKitList);
    disconnect(KitManager::instance(), &KitManager::kitsLoaded,
               this, &AndroidPlugin::kitsRestored);
}

} // namespace Internal
} // namespace Android
