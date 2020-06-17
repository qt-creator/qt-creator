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
#include "androidmanager.h"
#include "androidmanifesteditorfactory.h"
#include "androidpackageinstallationstep.h"
#include "androidpotentialkit.h"
#include "androidqmltoolingsupport.h"
#include "androidqtversion.h"
#include "androidrunconfiguration.h"
#include "androidruncontrol.h"
#include "androidsettingswidget.h"
#include "androidtoolchain.h"
#include "javaeditor.h"

#ifdef HAVE_QBS
#  include "androidqbspropertyprovider.h"
#endif

#include <coreplugin/icore.h>
#include <utils/checkablemessagebox.h>
#include <utils/infobar.h>

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

const char kSetupAndroidSetting[] = "ConfigureAndroid";

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
    }
};

class AndroidQmlPreviewWorker : public AndroidQmlToolingSupport
{
public:
    AndroidQmlPreviewWorker(RunControl *runControl)
        : AndroidQmlToolingSupport(runControl, runControl->runnable().executable.toString())
    {}
};

class AndroidPluginPrivate : public QObject
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

    RunWorkerFactory runWorkerFactory{
        RunWorkerFactory::make<AndroidRunSupport>(),
        {NORMAL_RUN_MODE},
        {runConfigFactory.runConfigurationId()}
    };
    RunWorkerFactory debugWorkerFactory{
        RunWorkerFactory::make<AndroidDebugSupport>(),
        {DEBUG_RUN_MODE},
        {runConfigFactory.runConfigurationId()}
    };
    RunWorkerFactory profilerWorkerFactory{
        RunWorkerFactory::make<AndroidQmlToolingSupport>(),
        {QML_PROFILER_RUN_MODE},
        {runConfigFactory.runConfigurationId()}
    };
    RunWorkerFactory qmlPreviewWorkerFactory{
        RunWorkerFactory::make<AndroidQmlToolingSupport>(),
        {QML_PREVIEW_RUN_MODE},
        {runConfigFactory.runConfigurationId()}
    };
    RunWorkerFactory qmlPreviewWorkerFactory2{
        RunWorkerFactory::make<AndroidQmlPreviewWorker>(),
        {QML_PREVIEW_RUN_MODE},
        {"QmlProjectManager.QmlRunConfiguration"},
        {Android::Constants::ANDROID_DEVICE_TYPE}
    };

    AndroidBuildApkStepFactory buildApkStepFactory;
};

AndroidPlugin::~AndroidPlugin()
{
    delete d;
}

bool AndroidPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    d = new AndroidPluginPrivate;

    connect(KitManager::instance(), &KitManager::kitsLoaded,
            this, &AndroidPlugin::kitsRestored);

    return true;
}

void AndroidPlugin::kitsRestored()
{
    const bool qtForAndroidInstalled
        = !QtSupport::QtVersionManager::versions([](const QtSupport::BaseQtVersion *v) {
               return v->targetDeviceTypes().contains(Android::Constants::ANDROID_DEVICE_TYPE);
           }).isEmpty();

    if (!AndroidConfigurations::currentConfig().sdkFullyConfigured() && qtForAndroidInstalled) {
        connect(Core::ICore::instance(), &Core::ICore::coreOpened, this,
                &AndroidPlugin::askUserAboutAndroidSetup, Qt::QueuedConnection);
    }

    AndroidConfigurations::registerNewToolChains();
    AndroidConfigurations::updateAutomaticKitList();
    connect(QtSupport::QtVersionManager::instance(), &QtSupport::QtVersionManager::qtVersionsChanged,
            AndroidConfigurations::instance(), []() {
        AndroidConfigurations::registerNewToolChains();
        AndroidConfigurations::updateAutomaticKitList();
    });
    disconnect(KitManager::instance(), &KitManager::kitsLoaded,
               this, &AndroidPlugin::kitsRestored);
}

void AndroidPlugin::askUserAboutAndroidSetup()
{
    if (!Utils::CheckableMessageBox::shouldAskAgain(Core::ICore::settings(), kSetupAndroidSetting)
        || !Core::ICore::infoBar()->canInfoBeAdded(kSetupAndroidSetting))
        return;

    Utils::InfoBarEntry
        info(kSetupAndroidSetting,
             tr("Would you like to configure Android options? This will ensure "
                "Android kits can be usable and all essential packages are installed. "
                "To do it later, select Options > Devices > Android."),
             Utils::InfoBarEntry::GlobalSuppression::Enabled);
    info.setCustomButtonInfo(tr("Configure Android"), [this] {
        Core::ICore::infoBar()->removeInfo(kSetupAndroidSetting);
        Core::ICore::infoBar()->globallySuppressInfo(kSetupAndroidSetting);
        QTimer::singleShot(0, this, [this]() { d->potentialKit.executeFromMenu(); });
    });
    Core::ICore::infoBar()->addInfo(info);
}

} // namespace Internal
} // namespace Android
