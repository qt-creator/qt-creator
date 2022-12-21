// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcusupportplugin.h"
#include "mcukitinformation.h"
#include "mcukitmanager.h"
#include "mcusupportconstants.h"
#include "mcusupportdevice.h"
#include "mcusupportoptions.h"
#include "mcusupportoptionspage.h"
#include "mcusupportrunconfiguration.h"

#if defined(WITH_TESTS) && defined(GOOGLE_TEST_IS_FOUND)
#include "test/unittest.h"
#endif

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/jsonwizard/jsonwizardfactory.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/infobar.h>

#include <QTimer>

using namespace Core;
using namespace ProjectExplorer;

namespace {
constexpr char setupMcuSupportKits[]{"SetupMcuSupportKits"};
}

namespace McuSupport {
namespace Internal {

void printMessage(const QString &message, bool important)
{
    const QString displayMessage = QCoreApplication::translate("QtForMCUs", "Qt for MCUs: %1")
                                       .arg(message);
    if (important)
        Core::MessageManager::writeFlashing(displayMessage);
    else
        Core::MessageManager::writeSilently(displayMessage);
}

class McuSupportPluginPrivate
{
public:
    McuSupportDeviceFactory deviceFactory;
    McuSupportRunConfigurationFactory runConfigurationFactory;
    RunWorkerFactory runWorkerFactory{makeFlashAndRunWorker(),
                                      {ProjectExplorer::Constants::NORMAL_RUN_MODE},
                                      {Constants::RUNCONFIGURATION}};
    SettingsHandler::Ptr m_settingsHandler{new SettingsHandler};
    McuSupportOptions m_options{m_settingsHandler};
    McuSupportOptionsPage optionsPage{m_options, m_settingsHandler};
    McuDependenciesKitAspect environmentPathsKitAspect;
}; // class McuSupportPluginPrivate

static McuSupportPluginPrivate *dd{nullptr};

McuSupportPlugin::~McuSupportPlugin()
{
    delete dd;
    dd = nullptr;
}

bool McuSupportPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    setObjectName("McuSupportPlugin");
    dd = new McuSupportPluginPrivate;

    dd->m_options.registerQchFiles();
    dd->m_options.registerExamples();
    ProjectExplorer::JsonWizardFactory::addWizardPath(":/mcusupport/wizards/");

    return true;
}

void McuSupportPlugin::extensionsInitialized()
{
    ProjectExplorer::DeviceManager::instance()->addDevice(McuSupportDevice::create());

    connect(KitManager::instance(), &KitManager::kitsLoaded, [this]() {
        McuKitManager::removeOutdatedKits();
        McuKitManager::createAutomaticKits(dd->m_settingsHandler);
        McuKitManager::fixExistingKits(dd->m_settingsHandler);
        askUserAboutMcuSupportKitsSetup();
    });
}

void McuSupportPlugin::askUserAboutMcuSupportKitsSetup()
{
    if (!ICore::infoBar()->canInfoBeAdded(setupMcuSupportKits)
        || dd->m_options.qulDirFromSettings().isEmpty()
        || !McuKitManager::existingKits(nullptr).isEmpty())
        return;

    Utils::InfoBarEntry info(setupMcuSupportKits,
                             tr("Create Kits for Qt for MCUs? "
                                "To do it later, select Edit > Preferences > Devices > MCU."),
                             Utils::InfoBarEntry::GlobalSuppression::Enabled);
    // clazy:excludeall=connect-3arg-lambda
    info.addCustomButton(tr("Create Kits for Qt for MCUs"), [] {
        ICore::infoBar()->removeInfo(setupMcuSupportKits);
        QTimer::singleShot(0, []() { ICore::showOptionsDialog(Constants::SETTINGS_ID); });
    });
    ICore::infoBar()->addInfo(info);
}

void McuSupportPlugin::askUserAboutMcuSupportKitsUpgrade(const SettingsHandler::Ptr &settingsHandler)
{
    const char upgradeMcuSupportKits[] = "UpgradeMcuSupportKits";

    if (!ICore::infoBar()->canInfoBeAdded(upgradeMcuSupportKits))
        return;

    Utils::InfoBarEntry info(upgradeMcuSupportKits,
                             tr("New version of Qt for MCUs detected. Upgrade existing Kits?"),
                             Utils::InfoBarEntry::GlobalSuppression::Enabled);
    using McuKitManager::UpgradeOption;
    static UpgradeOption selectedOption = UpgradeOption::Keep;

    const QList<Utils::InfoBarEntry::ComboInfo> infos
        = {{tr("Create new kits"), QVariant::fromValue(UpgradeOption::Keep)},
           {tr("Replace existing kits"), QVariant::fromValue(UpgradeOption::Replace)}};

    info.setComboInfo(infos, [](const Utils::InfoBarEntry::ComboInfo &selected) {
        selectedOption = selected.data.value<UpgradeOption>();
    });

    info.addCustomButton(tr("Proceed"), [upgradeMcuSupportKits, settingsHandler] {
        ICore::infoBar()->removeInfo(upgradeMcuSupportKits);
        QTimer::singleShot(0, [settingsHandler]() {
            McuKitManager::upgradeKitsByCreatingNewPackage(settingsHandler, selectedOption);
        });
    });

    ICore::infoBar()->addInfo(info);
}

QVector<QObject *> McuSupportPlugin::createTestObjects() const
{
    QVector<QObject *> tests;
#if defined(WITH_TESTS) && defined(GOOGLE_TEST_IS_FOUND)
    tests << new Test::McuSupportTest;
#endif
    return tests;
}

} // namespace Internal
} // namespace McuSupport
