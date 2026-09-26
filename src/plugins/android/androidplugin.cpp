// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidbuildapkstep.h"
#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androiddebugsupport.h"
#include "androiddeployqtstep.h"
#include "androiddevice.h"
#include "androidjavaclasspath.h"
#include "androidlogcatcrashparser.h"
#include "androidmcpsupport.h"
#include "androidpackageinstallationstep.h"
#include "androidqmltoolingsupport.h"
#include "androidqtversion.h"
#include "androidrunconfiguration.h"
#include "androidrunner.h"
#include "androidsettingswidget.h"
#include "androidtoolchain.h"
#include "androidtoolmenu.h"
#include "androidtr.h"

#ifdef WITH_TESTS
#  include "androidmanifestutils.h"
#  include "androidsdkmanager_test.h"
#  include "sdkmanageroutputparser_test.h"
#endif

#include <coreplugin/icore.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/environmentkitaspect.h>
#include <projectexplorer/javakitaspect.h>
#include <projectexplorer/kitmanager.h>

#include <qtsupport/qtversionmanager.h>

#include <nanotrace/nanotrace.h>

#include <utils/infobar.h>

#include <QTimer>

using namespace ProjectExplorer;

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

static void setupAndroidDeployConfiguration()
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

        ProjectExplorer::javaHomeCandidateProviders().append([](const Kit *k) -> Utils::FilePaths {
            // Only the kits whose JDK this plugin goes on to keep in step with
            // the setting. A kit of the user's own is theirs to point wherever
            // they like, and what the setting names reaches it through
            // AndroidToolchain, which exports it as it is at the time.
            if (RunDeviceTypeKitAspect::deviceTypeId(k) != Constants::ANDROID_DEVICE_TYPE
                || !k->detectionSource().isAutoDetected()) {
                return {};
            }
            return {AndroidConfig::openJDKLocation()};
        });

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
        setupAndroidLogcatCrashParser();

        setupAndroidJavaClassPath();
        setupAndroidToolsMenu();

        connect(KitManager::instance(), &KitManager::kitsLoaded, this, &AndroidPlugin::kitsRestored,
                Qt::SingleShotConnection);

        registerAndroidMcpTools();

#ifdef WITH_TESTS
        addTestCreator(createAndroidSdkManagerTest);
        addTestCreator(createAndroidSdkManagerOutputParserTest);
        addTestCreator(createAndroidQtVersionTest);
        addTestCreator(createAndroidConfigurationsTest);
        addTestCreator(createAndroidDeviceTest);
        addTestCreator(createAndroidLogcatCrashParserTest);
        addTestCreator(createAndroidMcpSupportTest);
        addTestCreator(createAndroidManifestUtilsTest);
        addTestCreator(createAndroidJavaClassPathTest);
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
        Utils::InfoBar *infoBar = Core::ICore::popupInfoBar();
        if (!infoBar->canInfoBeAdded(kSetupAndroidSetting))
            return;

        Utils::InfoBarEntry
            info(kSetupAndroidSetting,
                 Tr::tr("Automatically create usable Android kits and install all essential "
                        "packages. "
                        "To do this later, select Edit > Preferences > SDKs > Android."),
                 Utils::InfoBarEntry::GlobalSuppression::Enabled);
        info.setTitle(Tr::tr("Configure Android Options?"));
        info.setInfoType(Utils::InfoLabelType::Information);
        info.addCustomButton(
            Tr::tr("Configure Android"),
            [this] {
                QTimer::singleShot(0, this, [] {
                    Core::ICore::showSettings(Constants::ANDROID_SETTINGS_ID);
                });
            },
            {},
            Utils::InfoBarEntry::ButtonAction::SuppressPersistently);
        infoBar->addInfo(info);
    }
};

} // Android::Internal

#include "androidplugin.moc"
