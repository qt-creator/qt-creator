// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidbuildapkstep.h"
#include "androidconstants.h"
#include "androiddebugsupport.h"
#include "androiddeployqtstep.h"
#include "androiddevice.h"
#include "androidmanifesteditor.h"
#include "androidpackageinstallationstep.h"
#include "androidqmltoolingsupport.h"
#include "androidqtversion.h"
#include "androidrunconfiguration.h"
#include "androidrunner.h"
#include "androidsettingswidget.h"
#include "androidtoolchain.h"
#include "androidtr.h"

#ifdef WITH_TESTS
#  include "androidsdkmanager_test.h"
#  include "sdkmanageroutputparser_test.h"
#endif

#include "javaeditor.h"
#include "javalanguageserver.h"

#include <coreplugin/icore.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/environmentkitaspect.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtversionmanager.h>

#include <nanotrace/nanotrace.h>

#include <utils/infobar.h>

#include <QTimer>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Constants;

const char kSetupAndroidSetting[] = "ConfigureAndroid";

namespace Android::Internal {

class AndroidDeployConfigurationFactory final : public DeployConfigurationFactory
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

void setupAndroidDeployConfiguration()
{
    static AndroidDeployConfigurationFactory theAndroidDeployConfigurationFactory;
}

class AndroidPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Android.json")

    void initialize() final
    {
        setupAndroidConfigurations();

        setupAndroidDevice();
        setupAndroidQtVersion();
        setupAndroidToolchain();

        setupAndroidDeviceManager();

        setupAndroidSettingsPage();

        setupAndroidPackageInstallationStep();
        setupAndroidBuildApkStep();

        setupAndroidDeployConfiguration();
        setupAndroidDeployQtStep();

        setupAndroidRunConfiguration();
        setupAndroidRunWorker();
        setupAndroidDebugWorker();
        setupAndroidQmlToolingSupport();

        setupJavaEditor();
        setupAndroidManifestEditor();

        connect(KitManager::instance(), &KitManager::kitsLoaded, this, &AndroidPlugin::kitsRestored,
                Qt::SingleShotConnection);

        setupJavaLanguageServer();

#ifdef WITH_TESTS
        addTestCreator(createAndroidSdkManagerTest);
        addTestCreator(createAndroidSdkManagerOutputParserTest);
        addTestCreator(createAndroidQtVersionTest);
        addTestCreator(createAndroidConfigurationsTest);
#endif
    }

    void kitsRestored()
    {
        const bool qtForAndroidInstalled = !QtSupport::QtVersionManager::versions(
                                                &QtSupport::QtVersion::isAndroidQtVersion)
                                                .isEmpty();

        if (!AndroidConfig::sdkFullyConfigured() && qtForAndroidInstalled)
            askUserAboutAndroidSetup();

        AndroidConfigurations::registerNewToolchains();
        AndroidConfigurations::updateAutomaticKitList();
        connect(QtSupport::QtVersionManager::instance(), &QtSupport::QtVersionManager::qtVersionsChanged,
                AndroidConfigurations::instance(), [] {
                    AndroidConfigurations::registerNewToolchains();
                    AndroidConfigurations::updateAutomaticKitList();
                });
    }

    void askUserAboutAndroidSetup()
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
        info.addCustomButton(
            Tr::tr("Configure Android"),
            [this] {
                QTimer::singleShot(0, this, [] {
                    Core::ICore::showOptionsDialog(Constants::ANDROID_SETTINGS_ID);
                });
            },
            {},
            Utils::InfoBarEntry::ButtonAction::SuppressPersistently);
        Core::ICore::infoBar()->addInfo(info);
    }
};

} // Android::Internal

#include "androidplugin.moc"
