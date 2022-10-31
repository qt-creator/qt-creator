// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mcusupportoptions.h"

#include "mcuhelpers.h"
#include "mcukitmanager.h"
#include "mcupackage.h"
#include "mcusupportconstants.h"
#include "mcusupportsdk.h"
#include "mcutarget.h"
#include "settingshandler.h"

#include <cmakeprojectmanager/cmakekitinformation.h>
#include <cmakeprojectmanager/cmaketoolmanager.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <debugger/debuggerkitinformation.h>
#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>

#include <QMessageBox>
#include <QPushButton>

#include <utility>

using namespace Utils;

namespace McuSupport::Internal {

Macros *McuSdkRepository::globalMacros()
{
    static Macros macros;
    return &macros;
}

void McuSdkRepository::expandVariables()
{
    for (const auto &target : std::as_const(mcuTargets)) {
        auto macroExpander = getMacroExpander(*target);
        for (const auto &package : target->packages()) {
            package->setPath(macroExpander->expand(package->path()));
        }
    }
}

MacroExpanderPtr McuSdkRepository::getMacroExpander(const McuTarget &target)
{
    auto macroExpander = std::make_shared<Utils::MacroExpander>();

    //register the macros
    for (const auto &package : target.packages()) {
        macroExpander->registerVariable(package->cmakeVariableName().toLocal8Bit(),
                                        package->label(),
                                        [package] { return package->path().toString(); });
    }

    for (auto [key, macro] : asKeyValueRange(*globalMacros()))
        macroExpander->registerVariable(key.toLocal8Bit(), "QtMCUs Macro", macro);

    return macroExpander;
}

McuSupportOptions::McuSupportOptions(const SettingsHandler::Ptr &settingsHandler, QObject *parent)
    : QObject(parent)
    , qtForMCUsSdkPackage(createQtForMCUsPackage(settingsHandler))
    , settingsHandler(settingsHandler)
    , m_automaticKitCreation(settingsHandler->isAutomaticKitCreationEnabled())
{
    connect(qtForMCUsSdkPackage.get(),
            &McuAbstractPackage::changed,
            this,
            &McuSupportOptions::populatePackagesAndTargets);
}

void McuSupportOptions::populatePackagesAndTargets()
{
    setQulDir(qtForMCUsSdkPackage->path());
}

FilePath McuSupportOptions::qulDocsDir() const
{
    const FilePath qulDir = qulDirFromSettings();
    if (qulDir.isEmpty() || !qulDir.exists())
        return {};
    const FilePath docsDir = qulDir / "docs";
    return docsDir.exists() ? docsDir : FilePath();
}

void McuSupportOptions::registerQchFiles() const
{
    const QString docsDir = qulDocsDir().toString();
    if (docsDir.isEmpty())
        return;

    const QFileInfoList qchFiles = QDir(docsDir, "*.qch").entryInfoList();
    Core::HelpManager::registerDocumentation(
        Utils::transform<QStringList>(qchFiles,
                                      [](const QFileInfo &fi) { return fi.absoluteFilePath(); }));
}

void McuSupportOptions::registerExamples() const
{
    const FilePath docsDir = qulDocsDir();
    if (docsDir.isEmpty())
        return;

    auto examples = {std::make_pair(QStringLiteral("demos"), tr("Qt for MCUs Demos")),
                     std::make_pair(QStringLiteral("examples"), tr("Qt for MCUs Examples"))};
    for (const auto &dir : examples) {
        const FilePath examplesDir = qulDirFromSettings() / dir.first;
        if (!examplesDir.exists())
            continue;

        QtSupport::QtVersionManager::registerExampleSet(dir.second,
                                                        docsDir.toString(),
                                                        examplesDir.toString());
    }
}

const QVersionNumber &McuSupportOptions::minimalQulVersion()
{
    return minimalVersion;
}

bool McuSupportOptions::isLegacyVersion(const QVersionNumber &version)
{
    return version < newVersion;
}

void McuSupportOptions::setQulDir(const FilePath &path)
{
    //register the Qt installation directory containing Qul dir
    auto qtPath = (path / "../..").cleanPath();
    if (qtPath.exists()) {
        McuSdkRepository::globalMacros()->insert("QtDir", [qtPathString = qtPath.path()] {
            return qtPathString;
        });
    }
    qtForMCUsSdkPackage->updateStatus();
    if (qtForMCUsSdkPackage->isValidStatus())
        sdkRepository = targetsAndPackages(qtForMCUsSdkPackage, settingsHandler);
    else
        sdkRepository = McuSdkRepository{};
    for (const auto &package : std::as_const(sdkRepository.packages))
        connect(package.get(),
                &McuAbstractPackage::changed,
                this,
                &McuSupportOptions::packagesChanged);

    emit packagesChanged();
}

FilePath McuSupportOptions::qulDirFromSettings() const
{
    return settingsHandler->getPath(Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK,
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

void McuSupportOptions::checkUpgradeableKits()
{
    if (!qtForMCUsSdkPackage->isValidStatus() || sdkRepository.mcuTargets.isEmpty())
        return;

    if (Utils::anyOf(sdkRepository.mcuTargets, [this](const McuTargetPtr &target) {
            return !McuKitManager::upgradeableKits(target.get(), this->qtForMCUsSdkPackage).empty()
                   && McuKitManager::matchingKits(target.get(), this->qtForMCUsSdkPackage).empty();
        }))
        McuKitManager::upgradeKitsByCreatingNewPackage(settingsHandler, askForKitUpgrades());
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

} // namespace McuSupport::Internal
