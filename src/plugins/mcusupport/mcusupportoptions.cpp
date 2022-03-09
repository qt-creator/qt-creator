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

#include "mcusupportoptions.h"

#include "mcukitinformation.h"
#include "mcukitmanager.h"
#include "mcupackage.h"
#include "mcusupportconstants.h"
#include "mcusupportplugin.h"
#include "mcusupportsdk.h"
#include "mcutarget.h"

#include <cmakeprojectmanager/cmakekitinformation.h>
#include <cmakeprojectmanager/cmaketoolmanager.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <debugger/debuggerkitinformation.h>
#include <utils/algorithm.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>

#include <QMessageBox>
#include <QPushButton>

using CMakeProjectManager::CMakeConfigItem;
using CMakeProjectManager::CMakeConfigurationKitAspect;
using namespace ProjectExplorer;
using namespace Utils;

namespace McuSupport {
namespace Internal {

void McuSdkRepository::deletePackagesAndTargets()
{
    qDeleteAll(packages);
    packages.clear();
    mcuTargets.clear();
}

McuSupportOptions::McuSupportOptions(QObject *parent)
    : QObject(parent)
    , qtForMCUsSdkPackage(Sdk::createQtForMCUsPackage())
{
    connect(qtForMCUsSdkPackage,
            &McuAbstractPackage::changed,
            this,
            &McuSupportOptions::populatePackagesAndTargets);
    m_automaticKitCreation = automaticKitCreationFromSettings();
}

McuSupportOptions::~McuSupportOptions()
{
    deletePackagesAndTargets();
    delete qtForMCUsSdkPackage;
}

void McuSupportOptions::populatePackagesAndTargets()
{
    setQulDir(qtForMCUsSdkPackage->path());
}

static FilePath qulDocsDir()
{
    const FilePath qulDir = McuSupportOptions::qulDirFromSettings();
    if (qulDir.isEmpty() || !qulDir.exists())
        return {};
    const FilePath docsDir = qulDir.pathAppended("docs");
    return docsDir.exists() ? docsDir : FilePath();
}

void McuSupportOptions::registerQchFiles()
{
    const QString docsDir = qulDocsDir().toString();
    if (docsDir.isEmpty())
        return;

    const QFileInfoList qchFiles = QDir(docsDir, "*.qch").entryInfoList();
    Core::HelpManager::registerDocumentation(
        Utils::transform<QStringList>(qchFiles,
                                      [](const QFileInfo &fi) { return fi.absoluteFilePath(); }));
}

void McuSupportOptions::registerExamples()
{
    const FilePath docsDir = qulDocsDir();
    if (docsDir.isEmpty())
        return;

    auto examples = {std::make_pair(QStringLiteral("demos"), tr("Qt for MCUs Demos")),
                     std::make_pair(QStringLiteral("examples"), tr("Qt for MCUs Examples"))};
    for (const auto &dir : examples) {
        const FilePath examplesDir = McuSupportOptions::qulDirFromSettings().pathAppended(dir.first);
        if (!examplesDir.exists())
            continue;

        QtSupport::QtVersionManager::registerExampleSet(dir.second,
                                                        docsDir.toString(),
                                                        examplesDir.toString());
    }
}

const QVersionNumber &McuSupportOptions::minimalQulVersion()
{
    return legacyVersion;
}

void McuSupportOptions::setQulDir(const FilePath &dir)
{
    deletePackagesAndTargets();
    qtForMCUsSdkPackage->updateStatus();
    if (qtForMCUsSdkPackage->isValidStatus())
        Sdk::targetsAndPackages(dir, &sdkRepository);
    for (const auto &package : qAsConst(sdkRepository.packages))
        connect(package, &McuAbstractPackage::changed, this, &McuSupportOptions::packagesChanged);

    emit packagesChanged();
}

FilePath McuSupportOptions::qulDirFromSettings()
{
    return Sdk::packagePathFromSettings(Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK,
                                        QSettings::UserScope,
                                        {});
}

McuKitManager::UpgradeOption McuSupportOptions::askForKitUpgrades()
{
    QMessageBox upgradePopup(Core::ICore::dialogParent());
    upgradePopup.setStandardButtons(QMessageBox::Cancel);
    QPushButton *replaceButton = upgradePopup.addButton(tr("Replace Existing Kits"),
                                                        QMessageBox::NoRole);
    QPushButton *keepButton = upgradePopup.addButton(tr("Create New Kits"), QMessageBox::NoRole);
    upgradePopup.setWindowTitle(tr("Qt for MCUs"));
    upgradePopup.setText(tr("New version of Qt for MCUs detected. Upgrade existing kits?"));

    upgradePopup.exec();

    if (upgradePopup.clickedButton() == keepButton)
        return McuKitManager::UpgradeOption::Keep;

    if (upgradePopup.clickedButton() == replaceButton)
        return McuKitManager::UpgradeOption::Replace;

    return McuKitManager::UpgradeOption::Ignore;
}

void McuSupportOptions::deletePackagesAndTargets()
{
    sdkRepository.deletePackagesAndTargets();
}

void McuSupportOptions::checkUpgradeableKits()
{
    if (!qtForMCUsSdkPackage->isValidStatus() || sdkRepository.mcuTargets.size() == 0)
        return;

    if (Utils::anyOf(sdkRepository.mcuTargets, [this](const McuTarget *target) {
            return !McuKitManager::upgradeableKits(target, this->qtForMCUsSdkPackage).empty()
                   && McuKitManager::matchingKits(target, this->qtForMCUsSdkPackage).empty();
        }))
        McuKitManager::upgradeKitsByCreatingNewPackage(askForKitUpgrades());
}

bool McuSupportOptions::kitsNeedQtVersion()
{
    // Only on Windows, Qt is linked into the distributed qul Desktop libs. Also, the host tools
    // are missing the Qt runtime libraries on non-Windows.
    return !HostOsInfo::isWindowsHost();
}

bool McuSupportOptions::automaticKitCreationEnabled() const
{
    return m_automaticKitCreation;
}

void McuSupportOptions::setAutomaticKitCreationEnabled(const bool enabled)
{
    m_automaticKitCreation = enabled;
}

void McuSupportOptions::writeGeneralSettings() const
{
    const QString key = QLatin1String(Constants::SETTINGS_GROUP) + '/'
                        + QLatin1String(Constants::SETTINGS_KEY_AUTOMATIC_KIT_CREATION);
    QSettings *settings = Core::ICore::settings(QSettings::UserScope);
    settings->setValue(key, m_automaticKitCreation);
}

bool McuSupportOptions::automaticKitCreationFromSettings()
{
    QSettings *settings = Core::ICore::settings(QSettings::UserScope);
    const QString key = QLatin1String(Constants::SETTINGS_GROUP) + '/'
                        + QLatin1String(Constants::SETTINGS_KEY_AUTOMATIC_KIT_CREATION);
    const bool automaticKitCreation = settings->value(key, true).toBool();
    return automaticKitCreation;
}

} // namespace Internal
} // namespace McuSupport
