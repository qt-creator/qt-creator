// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidplugin.h"

#include "androidconfigurations.h"
#include "androidbuildapkstep.h"
#include "androidconstants.h"
#include "androiddebugsupport.h"
#include "androiddeployqtstep.h"
#include "androiddevice.h"
#include "androidmanifesteditorfactory.h"
#include "androidpackageinstallationstep.h"
#include "androidpotentialkit.h"
#include "androidqmlpreviewworker.h"
#include "androidqmltoolingsupport.h"
#include "androidqtversion.h"
#include "androidrunconfiguration.h"
#include "androidruncontrol.h"
#include "androidsettingswidget.h"
#include "androidtoolchain.h"
#include "androidtr.h"

#ifdef WITH_TESTS
#  include "androidsdkmanager_test.h"
#  include "sdkmanageroutputparser_test.h"
#endif

#include "javaeditor.h"
#include "javalanguageserver.h"

#ifdef HAVE_QBS
#  include "androidqbspropertyprovider.h"
#endif

#include <coreplugin/icore.h>
#include <utils/infobar.h>

#include <languageclient/languageclientsettings.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtversionmanager.h>

#include <nanotrace/nanotrace.h>

#include <QTimer>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Constants;

const char kSetupAndroidSetting[] = "ConfigureAndroid";

namespace Android::Internal {

class AndroidDeployConfigurationFactory : public DeployConfigurationFactory
{
public:
    AndroidDeployConfigurationFactory()
    {
        setConfigBaseId("Qt4ProjectManager.AndroidDeployConfiguration2");
        addSupportedTargetDeviceType(Constants::ANDROID_DEVICE_TYPE);
        setDefaultDisplayName(Tr::tr("Deploy to Android Device"));
        addInitialStep(Constants::ANDROID_DEPLOY_QT_ID);
    }
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
    AndroidRunWorkerFactory runWorkerFactory;
    AndroidDebugWorkerFactory debugWorkerFactory;
    AndroidQmlToolingSupportFactory profilerWorkerFactory;
    AndroidQmlPreviewWorkerFactory qmlPreviewWorkerFactory;
    AndroidBuildApkStepFactory buildApkStepFactory;
    AndroidDeviceManager deviceManager;
};

AndroidPlugin::~AndroidPlugin()
{
    delete d;
}

void AndroidPlugin::initialize()
{
    d = new AndroidPluginPrivate;

    connect(KitManager::instance(), &KitManager::kitsLoaded,
            this, &AndroidPlugin::kitsRestored);

    LanguageClient::LanguageClientSettings::registerClientType(
        {Android::Constants::JLS_SETTINGS_ID,
         Tr::tr("Java Language Server"),
         [] { return new JLSSettings; }});

#ifdef WITH_TESTS
    addTest<AndroidSdkManagerTest>();
    addTest<SdkManagerOutputParserTest>();
#endif
}

void AndroidPlugin::kitsRestored()
{
    const bool qtForAndroidInstalled
        = !QtSupport::QtVersionManager::versions([](const QtSupport::QtVersion *v) {
               return v->targetDeviceTypes().contains(Android::Constants::ANDROID_DEVICE_TYPE);
           }).isEmpty();

    if (!AndroidConfigurations::currentConfig().sdkFullyConfigured() && qtForAndroidInstalled)
        askUserAboutAndroidSetup();

    AndroidConfigurations::registerNewToolChains();
    AndroidConfigurations::updateAutomaticKitList();
    connect(QtSupport::QtVersionManager::instance(), &QtSupport::QtVersionManager::qtVersionsChanged,
            AndroidConfigurations::instance(), [] {
        AndroidConfigurations::registerNewToolChains();
        AndroidConfigurations::updateAutomaticKitList();
    });
    disconnect(KitManager::instance(), &KitManager::kitsLoaded,
               this, &AndroidPlugin::kitsRestored);
}

void AndroidPlugin::askUserAboutAndroidSetup()
{
    NANOTRACE_SCOPE("Android", "AndroidPlugin::askUserAboutAndroidSetup");
    if (!Core::ICore::infoBar()->canInfoBeAdded(kSetupAndroidSetting))
        return;

    Utils::InfoBarEntry
            info(kSetupAndroidSetting,
                 Tr::tr("Would you like to configure Android options? This will ensure "
                        "Android kits can be usable and all essential packages are installed. "
                        "To do it later, select Edit > Preferences > Devices > Android."),
                 Utils::InfoBarEntry::GlobalSuppression::Enabled);
    info.addCustomButton(Tr::tr("Configure Android"), [this] {
        Core::ICore::infoBar()->removeInfo(kSetupAndroidSetting);
        Core::ICore::infoBar()->globallySuppressInfo(kSetupAndroidSetting);
        QTimer::singleShot(0, this, [this] { d->potentialKit.executeFromMenu(); });
    });
    Core::ICore::infoBar()->addInfo(info);
}

} // Android::Internal
