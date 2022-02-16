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

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/jsonwizard/jsonwizardfactory.h>
#include <projectexplorer/kitmanager.h>

#include <utils/infobar.h>

#include <QTimer>

using namespace Core;
using namespace ProjectExplorer;

namespace McuSupport {
namespace Internal {

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

static McuSupportPluginPrivate* dd{nullptr};

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
        McuSupportOptions::removeOutdatedKits();
        McuSupportOptions::createAutomaticKits();
        McuSupportOptions::fixExistingKits();
        McuSupportPlugin::askUserAboutMcuSupportKitsSetup();
    });
}

void McuSupportPlugin::askUserAboutMcuSupportKitsSetup()
{
    const char setupMcuSupportKits[] = "SetupMcuSupportKits";

    if (!ICore::infoBar()->canInfoBeAdded(setupMcuSupportKits)
        || McuSupportOptions::qulDirFromSettings().isEmpty()
        || !McuSupportOptions::existingKits(nullptr).isEmpty())
        return;

    Utils::InfoBarEntry info(setupMcuSupportKits,
                             tr("Create Kits for Qt for MCUs? "
                                "To do it later, select Options > Devices > MCU."),
                             Utils::InfoBarEntry::GlobalSuppression::Enabled);
    info.addCustomButton(tr("Create Kits for Qt for MCUs"), [setupMcuSupportKits] {
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

    static McuSupportOptions::UpgradeOption selectedOption;
    const QStringList options = {tr("Create new kits"), tr("Replace existing kits")};
    selectedOption = McuSupportOptions::UpgradeOption::Keep;
    info.setComboInfo(options, [options](const QString &selected) {
        selectedOption = options.indexOf(selected) == 0 ? McuSupportOptions::UpgradeOption::Keep
                                                        : McuSupportOptions::UpgradeOption::Replace;
    });

    info.addCustomButton(tr("Proceed"), [upgradeMcuSupportKits] {
        ICore::infoBar()->removeInfo(upgradeMcuSupportKits);
        QTimer::singleShot(0, []() { McuSupportOptions::upgradeKits(selectedOption); });
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
