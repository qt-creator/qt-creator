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

#include "mcupackage.h"
#include "mcutarget.h"
#include "mcukitmanager.h"
#include "mcukitinformation.h"
#include "mcusupportcmakemapper.h"
#include "mcusupportconstants.h"
#include "mcusupportsdk.h"
#include "mcusupportplugin.h"

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
    qDeleteAll(mcuTargets);
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
    static const QVersionNumber v({2, 0});
    return v;
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
                                        QSettings::UserScope, {});
}

void McuSupportOptions::remapQul2xCmakeVars(Kit *kit, const EnvironmentItems &envItems)
{
    const auto cmakeVars = mapEnvVarsToQul2xCmakeVars(envItems);
    const auto cmakeVarNames = Utils::transform(cmakeVars, &CMakeConfigItem::key);

    // First filter out all Qul2.x CMake vars
    auto config = Utils::filtered(CMakeConfigurationKitAspect::configuration(kit),
                                  [&](const auto &configItem) {
                                      return !cmakeVarNames.contains(configItem.key);
                                  });
    // Then append them with new values
    config.append(cmakeVars);
    CMakeConfigurationKitAspect::setConfiguration(kit, config);
}

static void setKitToolchains(Kit *k, const McuToolChainPackage *tcPackage)
{
    switch (tcPackage->toolchainType()) {
    case McuToolChainPackage::ToolChainType::Unsupported:
        return;

    case McuToolChainPackage::ToolChainType::GHS:
    case McuToolChainPackage::ToolChainType::GHSArm:
        return; // No Green Hills toolchain, because support for it is missing.

    case McuToolChainPackage::ToolChainType::IAR:
    case McuToolChainPackage::ToolChainType::KEIL:
    case McuToolChainPackage::ToolChainType::MSVC:
    case McuToolChainPackage::ToolChainType::GCC:
    case McuToolChainPackage::ToolChainType::ArmGcc:
        ToolChainKitAspect::setToolChain(k,
                                         tcPackage->toolChain(
                                             ProjectExplorer::Constants::C_LANGUAGE_ID));
        ToolChainKitAspect::setToolChain(k,
                                         tcPackage->toolChain(
                                             ProjectExplorer::Constants::CXX_LANGUAGE_ID));
        return;

    default:
        Q_UNREACHABLE();
    }
}

static void setKitDebugger(Kit *k, const McuToolChainPackage *tcPackage)
{
    if (tcPackage->isDesktopToolchain()) {
        // Qt Creator seems to be smart enough to deduce the right Kit debugger from the ToolChain
        return;
    }

    switch (tcPackage->toolchainType()) {
    case McuToolChainPackage::ToolChainType::Unsupported:
    case McuToolChainPackage::ToolChainType::GHS:
    case McuToolChainPackage::ToolChainType::GHSArm:
    case McuToolChainPackage::ToolChainType::IAR:
        return; // No Green Hills and IAR debugger, because support for it is missing.

    case McuToolChainPackage::ToolChainType::KEIL:
    case McuToolChainPackage::ToolChainType::MSVC:
    case McuToolChainPackage::ToolChainType::GCC:
    case McuToolChainPackage::ToolChainType::ArmGcc: {
        const QVariant debuggerId = tcPackage->debuggerId();
        if (debuggerId.isValid()) {
            Debugger::DebuggerKitAspect::setDebugger(k, debuggerId);
        }
        return;
    }

    default:
        Q_UNREACHABLE();
    }
}

static void setKitDevice(Kit *k, const McuTarget *mcuTarget)
{
    // "Device Type" Desktop is the default. We use that for the Qt for MCUs Desktop Kit
    if (mcuTarget->toolChainPackage()->isDesktopToolchain())
        return;

    DeviceTypeKitAspect::setDeviceTypeId(k, Constants::DEVICE_TYPE);
}

static bool expectsCmakeVars(const McuTarget *mcuTarget)
{
    return mcuTarget->qulVersion() >= QVersionNumber{2, 0};
}

void McuSupportOptions::setKitEnvironment(Kit *k,
                                          const McuTarget *mcuTarget,
                                          const McuAbstractPackage *qtForMCUsSdkPackage)
{
    EnvironmentItems changes;
    QStringList pathAdditions;

    // The Desktop version depends on the Qt shared libs in Qul_DIR/bin.
    // If CMake's fileApi is avaialble, we can rely on the "Add library search path to PATH"
    // feature of the run configuration. Otherwise, we just prepend the path, here.
    if (mcuTarget->toolChainPackage()->isDesktopToolchain()
        && !CMakeProjectManager::CMakeToolManager::defaultCMakeTool()->hasFileApi())
        pathAdditions.append(qtForMCUsSdkPackage->path().pathAppended("bin").toUserOutput());

    auto processPackage = [&pathAdditions, &changes](const McuAbstractPackage *package) {
        if (package->isAddToSystemPath())
            pathAdditions.append(package->path().toUserOutput());
        if (!package->environmentVariableName().isEmpty())
            changes.append({package->environmentVariableName(), package->path().toUserOutput()});
    };
    for (auto package : mcuTarget->packages())
        processPackage(package);
    processPackage(qtForMCUsSdkPackage);

    if (McuSupportOptions::kitsNeedQtVersion())
        changes.append({QLatin1String("LD_LIBRARY_PATH"), "%{Qt:QT_INSTALL_LIBS}"});

    // Hack, this problem should be solved in lower layer
    if (expectsCmakeVars(mcuTarget)) {
        McuSupportOptions::remapQul2xCmakeVars(k, changes);
    }

    EnvironmentKitAspect::setEnvironmentChanges(k, changes);
}

static void setKitDependencies(Kit *k,
                               const McuTarget *mcuTarget,
                               const McuAbstractPackage *qtForMCUsSdkPackage)
{
    NameValueItems dependencies;

    auto processPackage = [&dependencies](const McuAbstractPackage *package) {
        if (!package->environmentVariableName().isEmpty())
            dependencies.append({package->environmentVariableName(),
                                 package->detectionPath().toUserOutput()});
    };
    for (auto package : mcuTarget->packages())
        processPackage(package);
    processPackage(qtForMCUsSdkPackage);

    McuDependenciesKitAspect::setDependencies(k, dependencies);

    auto irrelevant = k->irrelevantAspects();
    irrelevant.insert(McuDependenciesKitAspect::id());
    k->setIrrelevantAspects(irrelevant);
}

void McuSupportOptions::updateKitEnvironment(Kit *k, const McuTarget *mcuTarget)
{
    EnvironmentItems changes = EnvironmentKitAspect::environmentChanges(k);
    for (auto package : mcuTarget->packages()) {
        const QString varName = package->environmentVariableName();
        if (!varName.isEmpty() && package->isValidStatus()) {
            const int index = Utils::indexOf(changes, [varName](const EnvironmentItem &item) {
                return item.name == varName;
            });
            const EnvironmentItem item = {package->environmentVariableName(),
                                          package->path().toUserOutput()};
            if (index != -1)
                changes.replace(index, item);
            else
                changes.append(item);
        }
    }

    // Hack, this problem should be solved in lower layer
    if (expectsCmakeVars(mcuTarget)) {
        remapQul2xCmakeVars(k, changes);
    }

    EnvironmentKitAspect::setEnvironmentChanges(k, changes);
}

static void setKitCMakeOptions(Kit *k, const McuTarget *mcuTarget, const FilePath &qulDir)
{
    using namespace CMakeProjectManager;

    CMakeConfig config = CMakeConfigurationKitAspect::configuration(k);
    // CMake ToolChain file for ghs handles CMAKE_*_COMPILER autonomously
    if (mcuTarget->toolChainPackage()->toolchainType() != McuToolChainPackage::ToolChainType::GHS
        && mcuTarget->toolChainPackage()->toolchainType() != McuToolChainPackage::ToolChainType::GHSArm) {
        config.append(CMakeConfigItem("CMAKE_CXX_COMPILER", "%{Compiler:Executable:Cxx}"));
        config.append(CMakeConfigItem("CMAKE_C_COMPILER", "%{Compiler:Executable:C}"));
    }

    if (!mcuTarget->toolChainPackage()->isDesktopToolchain()) {
        const FilePath cMakeToolchainFile = qulDir.pathAppended(
            "lib/cmake/Qul/toolchain/" + mcuTarget->toolChainPackage()->cmakeToolChainFileName());

        config.append(
            CMakeConfigItem("CMAKE_TOOLCHAIN_FILE", cMakeToolchainFile.toString().toUtf8()));
        if (!cMakeToolchainFile.exists()) {
            printMessage(McuTarget::tr(
                             "Warning for target %1: missing CMake toolchain file expected at %2.")
                             .arg(McuKitManager::kitName(mcuTarget),
                                  cMakeToolchainFile.toUserOutput()),
                         false);
        }
    }

    const FilePath generatorsPath = qulDir.pathAppended("/lib/cmake/Qul/QulGenerators.cmake");
    config.append(CMakeConfigItem("QUL_GENERATORS", generatorsPath.toString().toUtf8()));
    if (!generatorsPath.exists()) {
        printMessage(McuTarget::tr("Warning for target %1: missing QulGenerators expected at %2.")
                         .arg(McuKitManager::kitName(mcuTarget), generatorsPath.toUserOutput()),
                     false);
    }

    config.append(CMakeConfigItem("QUL_PLATFORM", mcuTarget->platform().name.toUtf8()));

    if (mcuTarget->colorDepth() != McuTarget::UnspecifiedColorDepth)
        config.append(CMakeConfigItem("QUL_COLOR_DEPTH",
                                      QString::number(mcuTarget->colorDepth()).toLatin1()));
    if (McuSupportOptions::kitsNeedQtVersion())
        config.append(CMakeConfigItem("CMAKE_PREFIX_PATH", "%{Qt:QT_INSTALL_PREFIX}"));
    CMakeConfigurationKitAspect::setConfiguration(k, config);

    if (HostOsInfo::isWindowsHost()) {
        auto type = mcuTarget->toolChainPackage()->toolchainType();
        if (type == McuToolChainPackage::ToolChainType::GHS || type == McuToolChainPackage::ToolChainType::GHSArm) {
            // See https://bugreports.qt.io/browse/UL-4247?focusedCommentId=565802&page=com.atlassian.jira.plugin.system.issuetabpanels:comment-tabpanel#comment-565802
            // and https://bugreports.qt.io/browse/UL-4247?focusedCommentId=565803&page=com.atlassian.jira.plugin.system.issuetabpanels:comment-tabpanel#comment-565803
            CMakeGeneratorKitAspect::setGenerator(k, "NMake Makefiles JOM");
        }
    }
}

static void setKitQtVersionOptions(Kit *k)
{
    if (!McuSupportOptions::kitsNeedQtVersion())
        QtSupport::QtKitAspect::setQtVersion(k, nullptr);
    // else: auto-select a Qt version
}

static FilePath kitDependencyPath(const Kit *kit, const QString &variableName)
{
    for (const NameValueItem &nameValueItem : EnvironmentKitAspect::environmentChanges(kit)) {
        if (nameValueItem.name == variableName)
            return FilePath::fromUserInput(nameValueItem.value);
    }
    return FilePath();
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
    if (!qtForMCUsSdkPackage->isValidStatus() || sdkRepository.mcuTargets.length() == 0)
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
