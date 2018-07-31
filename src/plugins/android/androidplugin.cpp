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
#include "androiddeployconfiguration.h"
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
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtversionmanager.h>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Constants;

namespace Android {
namespace Internal {

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

class AndroidPluginPrivate
{
public:
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
