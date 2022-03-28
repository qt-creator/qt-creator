/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
    McuSupportOptionsPage optionsPage;
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

    McuSupportOptions::registerQchFiles();
    McuSupportOptions::registerExamples();
    ProjectExplorer::JsonWizardFactory::addWizardPath(":/mcusupport/wizards/");

    return true;
}

void McuSupportPlugin::extensionsInitialized()
{
    ProjectExplorer::DeviceManager::instance()->addDevice(McuSupportDevice::create());

    connect(KitManager::instance(), &KitManager::kitsLoaded, []() {
        McuKitManager::removeOutdatedKits();
        McuKitManager::createAutomaticKits();
        McuKitManager::fixExistingKits();
        McuSupportPlugin::askUserAboutMcuSupportKitsSetup();
    });
}

void McuSupportPlugin::askUserAboutMcuSupportKitsSetup()
{
    if (!ICore::infoBar()->canInfoBeAdded(setupMcuSupportKits)
        || McuSupportOptions::qulDirFromSettings().isEmpty()
        || !McuKitManager::existingKits(nullptr).isEmpty())
        return;

    Utils::InfoBarEntry info(setupMcuSupportKits,
                             tr("Create Kits for Qt for MCUs? "
                                "To do it later, select Options > Devices > MCU."),
                             Utils::InfoBarEntry::GlobalSuppression::Enabled);
    // clazy:excludeall=connect-3arg-lambda
    info.addCustomButton(tr("Create Kits for Qt for MCUs"), [] {
        ICore::infoBar()->removeInfo(setupMcuSupportKits);
        QTimer::singleShot(0, []() { ICore::showOptionsDialog(Constants::SETTINGS_ID); });
    });
    ICore::infoBar()->addInfo(info);
}

void McuSupportPlugin::askUserAboutMcuSupportKitsUpgrade()
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

    info.addCustomButton(tr("Proceed"), [upgradeMcuSupportKits] {
        ICore::infoBar()->removeInfo(upgradeMcuSupportKits);
        QTimer::singleShot(0, []() {
            McuKitManager::upgradeKitsByCreatingNewPackage(selectedOption);
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
