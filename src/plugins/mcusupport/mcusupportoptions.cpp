// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcusupportoptions.h"

#include "dialogs/mcukitcreationdialog.h"
#include "mcuhelpers.h"
#include "mcukitmanager.h"
#include "mcupackage.h"
#include "mcusupport_global.h"
#include "mcusupportconstants.h"
#include "mcusupportsdk.h"
#include "mcusupporttr.h"
#include "mcutarget.h"
#include "settingshandler.h"

#include <cmakeprojectmanager/cmakekitaspect.h>
#include <cmakeprojectmanager/cmaketoolmanager.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <debugger/debuggerkitaspect.h>
#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/infobar.h>
#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtversionmanager.h>

#include <QMessageBox>
#include <QPushButton>

#include <utility>

using namespace Utils;

namespace McuSupport::Internal {

// Utils::FileFilter do not support globbing with "*" placed in the middle of the path,
// since it is required for paths such as "Microsoft Visual Studio/2019/*/VC/Tools/MSVC/*/bin/Hostx64/x64"
// The filter is applied for each time a wildcard character is found in a path component.
// Returns a pair of the longest path if multiple ones exists and the number of components that were not found.
static const std::pair<Utils::FilePath, int> expandWildcards(
    const FilePath path, const QList<QStringView> patternComponents)
{
    // Only absolute paths are currently supported
    // Call FilePath::cleanPath on the path before calling this function
    if (!path.exists() || path.isRelativePath())
        return {path, patternComponents.size()};

    // All components are found
    if (patternComponents.empty())
        return {path, patternComponents.size()};

    const QString currentComponent = patternComponents.front().toString();
    FilePath currentPath = path / currentComponent;

    if (!currentComponent.contains("*") && !currentComponent.contains("?") && currentPath.exists())
        return expandWildcards(path / currentComponent,
                               {patternComponents.constBegin() + 1, patternComponents.constEnd()});

    auto entries = path.dirEntries(
        Utils::FileFilter({currentComponent}, QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot));

    std::pair<FilePath, int> retPair = {path, patternComponents.size()};

    sort(entries, [](const FilePath &a, const FilePath &b) { return a.fileName() < b.fileName(); });
    for (const auto &entry : entries) {
        auto [entry_path, remaining_components] = expandWildcards(entry,
                                                                  {patternComponents.constBegin()
                                                                       + 1,
                                                                   patternComponents.constEnd()});
        if (remaining_components <= retPair.second)
            retPair = {entry_path, remaining_components};
    }

    return retPair;
}

Macros *McuSdkRepository::globalMacros()
{
    static Macros macros;
    return &macros;
}

void McuSdkRepository::updateQtDirMacro(const FilePath &qulDir)
{
    // register the Qt installation directory containing Qul dir
    auto qtPath = (qulDir / "../..").cleanPath();
    if (qtPath.exists()) {
        globalMacros()->insert("QtDir", [qtPathString = qtPath.path()] { return qtPathString; });
    }
}

void McuSdkRepository::expandVariablesAndWildcards()
{
    for (const auto &target : std::as_const(mcuTargets)) {
        auto macroExpander = getMacroExpander(*target);
        for (const auto &package : target->packages()) {
            // Expand variables
            const auto path = macroExpander->expand(package->path());

            //expand wildcards
            // Ignore expanding if no wildcards are found
            if (!path.path().contains("*") && !path.path().contains("?")) {
                package->setPath(path);
                continue;
            }

            QStringList pathComponents = path.cleanPath().path().split("/");

            // Path components example on linux: {"", "home", "username"}
            // Path components example on windows: {"C:", "Users", "username"}
            // 2 for empty_split_entry(linux)|root(windows) + at least one component
            if (pathComponents.size() < 2) {
                package->setPath(path);
                continue;
            }
            // drop empty_split_entry(linux)|root(windows)
            QString root = pathComponents.takeFirst();
            root.append('/'); // ensure we have a path (UNIX just a '/', Windows 'C:/' or similar)

            package->setPath(
                expandWildcards(FilePath::fromString(root),
                                {pathComponents.constBegin(), pathComponents.constEnd()})
                    .first);
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

    auto examples = {std::make_pair(QStringLiteral("demos"), Tr::tr("Qt for MCUs Demos")),
                     std::make_pair(QStringLiteral("examples"), Tr::tr("Qt for MCUs Examples"))};
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
    McuSdkRepository::updateQtDirMacro(path);
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
    QPushButton *replaceButton = upgradePopup.addButton(Tr::tr("Replace Existing Kits"),
                                                        QMessageBox::NoRole);
    QPushButton *keepButton = upgradePopup.addButton(Tr::tr("Create New Kits"), QMessageBox::NoRole);
    upgradePopup.setWindowTitle(Tr::tr("Qt for MCUs"));
    upgradePopup.setText(Tr::tr("New version of Qt for MCUs detected. Upgrade existing kits?"));

    upgradePopup.exec();

    if (upgradePopup.clickedButton() == keepButton)
        return McuKitManager::UpgradeOption::Keep;

    if (upgradePopup.clickedButton() == replaceButton)
        return McuKitManager::UpgradeOption::Replace;

    return McuKitManager::UpgradeOption::Ignore;
}

void McuSupportOptions::displayKitCreationMessages(const MessagesList &messages,
                                                   const SettingsHandler::Ptr &settingsHandler,
                                                   McuPackagePtr qtMCUsPackage)
{
    if (messages.isEmpty() || !qtMCUsPackage->isValidStatus())
        return;
    static const char mcuKitCreationErrorInfoId[] = "ErrorWhileCreatingMCUKits";
    if (!Core::ICore::infoBar()->canInfoBeAdded(mcuKitCreationErrorInfoId))
        return;

    Utils::InfoBarEntry info(mcuKitCreationErrorInfoId,
                             Tr::tr("Errors while creating Qt for MCUs kits"),
                             Utils::InfoBarEntry::GlobalSuppression::Enabled);

    info.addCustomButton(Tr::tr("Details"), [=] {
        auto popup = new McuKitCreationDialog(messages, settingsHandler, qtMCUsPackage);
        popup->exec();
        delete popup;
        Core::ICore::infoBar()->removeInfo(mcuKitCreationErrorInfoId);
    });

    Core::ICore::infoBar()->addInfo(info);
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
