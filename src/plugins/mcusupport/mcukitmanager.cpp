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

#include "mcukitmanager.h"
#include "mcusupportoptions.h"

#include "mcusupportconstants.h"
#include "mcukitinformation.h"
#include "mcupackage.h"
#include "mcutarget.h"
#include "mcusupportplugin.h"
#include "mcusupportsdk.h"
#include "mcusupportcmakemapper.h"

#include <cmakeprojectmanager/cmakekitinformation.h>
#include <cmakeprojectmanager/cmaketoolmanager.h>
#include <coreplugin/icore.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggeritemmanager.h>
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
namespace McuKitManager {

static const int KIT_VERSION = 9; // Bumps up whenever details in Kit creation change

static bool expectsCmakeVars(const McuTarget *mcuTarget)
{
    return mcuTarget->qulVersion() >= QVersionNumber{2, 0};
}

void remapQul2xCmakeVars(Kit *kit, const EnvironmentItems &envItems)
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

static void setKitProperties(const QString &kitName,
                             Kit *k,
                             const McuTarget *mcuTarget,
                             const FilePath &sdkPath)
{
    using namespace Constants;

    k->setUnexpandedDisplayName(kitName);
    k->setValue(KIT_MCUTARGET_VENDOR_KEY, mcuTarget->platform().vendor);
    k->setValue(KIT_MCUTARGET_MODEL_KEY, mcuTarget->platform().name);
    k->setValue(KIT_MCUTARGET_COLORDEPTH_KEY, mcuTarget->colorDepth());
    k->setValue(KIT_MCUTARGET_SDKVERSION_KEY, mcuTarget->qulVersion().toString());
    k->setValue(KIT_MCUTARGET_KITVERSION_KEY, KIT_VERSION);
    k->setValue(KIT_MCUTARGET_OS_KEY, static_cast<int>(mcuTarget->os()));
    k->setValue(KIT_MCUTARGET_TOOCHAIN_KEY, mcuTarget->toolChainPackage()->toolChainName());
    k->setAutoDetected(false);
    k->makeSticky();
    if (mcuTarget->toolChainPackage()->isDesktopToolchain())
        k->setDeviceTypeForIcon(DEVICE_TYPE);
    k->setValue(QtSupport::SuppliesQtQuickImportPath::id(), true);
    k->setValue(QtSupport::KitQmlImportPath::id(), sdkPath.pathAppended("include/qul").toVariant());
    k->setValue(QtSupport::KitHasMergedHeaderPathsWithQmlImportPaths::id(), true);
    QSet<Id> irrelevant = {
        SysRootKitAspect::id(),
        QtSupport::SuppliesQtQuickImportPath::id(),
        QtSupport::KitQmlImportPath::id(),
        QtSupport::KitHasMergedHeaderPathsWithQmlImportPaths::id(),
    };
    if (!McuSupportOptions::kitsNeedQtVersion())
        irrelevant.insert(QtSupport::QtKitAspect::id());
    k->setIrrelevantAspects(irrelevant);
}


static void setKitEnvironment(Kit *k,
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
        remapQul2xCmakeVars(k, changes);
    }

    EnvironmentKitAspect::setEnvironmentChanges(k, changes);
}

void updateKitEnvironment(Kit *k, const McuTarget *mcuTarget)
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

static void setKitDependencies(Kit *k,
                               const McuTarget *mcuTarget,
                               const McuAbstractPackage *qtForMCUsSdkPackage)
{
    NameValueItems dependencies;

    auto processPackage = [&dependencies](const McuAbstractPackage *package) {
        const auto cmakeVariableName = mapEnvVarToQul2xCmakeVar(package->environmentVariableName());
        if (!cmakeVariableName.isEmpty())
            dependencies.append({cmakeVariableName,
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
                             .arg(kitName(mcuTarget),
                                  cMakeToolchainFile.toUserOutput()),
                         false);
        }
    }

    const FilePath generatorsPath = qulDir.pathAppended("/lib/cmake/Qul/QulGenerators.cmake");
    config.append(CMakeConfigItem("QUL_GENERATORS", generatorsPath.toString().toUtf8()));
    if (!generatorsPath.exists()) {
        printMessage(McuTarget::tr("Warning for target %1: missing QulGenerators expected at %2.")
                         .arg(kitName(mcuTarget), generatorsPath.toUserOutput()),
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

QString kitName(const McuTarget *mcuTarget)
{
    const McuToolChainPackage *tcPkg = mcuTarget->toolChainPackage();
    const QString compilerName = tcPkg && !tcPkg->isDesktopToolchain()
                                     ? QString::fromLatin1(" (%1)").arg(
                                         tcPkg->toolChainName().toUpper())
                                     : "";
    const QString colorDepth = mcuTarget->colorDepth() != McuTarget::UnspecifiedColorDepth
                                   ? QString::fromLatin1(" %1bpp").arg(mcuTarget->colorDepth())
                                   : "";
    const QString targetName = mcuTarget->platform().displayName.isEmpty()
                                   ? mcuTarget->platform().name
                                   : mcuTarget->platform().displayName;
    return QString::fromLatin1("Qt for MCUs %1.%2 - %3%4%5")
        .arg(QString::number(mcuTarget->qulVersion().majorVersion()),
             QString::number(mcuTarget->qulVersion().minorVersion()),
             targetName,
             colorDepth,
             compilerName);
}

QList<Kit *> existingKits(const McuTarget *mcuTarget)
{
    using namespace Constants;
    return Utils::filtered(KitManager::kits(), [mcuTarget](Kit *kit) {
        return kit->value(KIT_MCUTARGET_KITVERSION_KEY) == KIT_VERSION
               && (!mcuTarget
                   || (kit->value(KIT_MCUTARGET_VENDOR_KEY) == mcuTarget->platform().vendor
                       && kit->value(KIT_MCUTARGET_MODEL_KEY) == mcuTarget->platform().name
                       && kit->value(KIT_MCUTARGET_COLORDEPTH_KEY) == mcuTarget->colorDepth()
                       && kit->value(KIT_MCUTARGET_OS_KEY).toInt()
                              == static_cast<int>(mcuTarget->os())
                       && kit->value(KIT_MCUTARGET_TOOCHAIN_KEY)
                              == mcuTarget->toolChainPackage()->toolChainName()));
    });
}

QList<Kit *> matchingKits(const McuTarget *mcuTarget,
                                             const McuAbstractPackage *qtForMCUsSdkPackage)
{
    return Utils::filtered(existingKits(mcuTarget), [mcuTarget, qtForMCUsSdkPackage](Kit *kit) {
        return kitIsUpToDate(kit, mcuTarget, qtForMCUsSdkPackage);
    });
}

QList<Kit *> upgradeableKits(const McuTarget *mcuTarget,
                                                const McuAbstractPackage *qtForMCUsSdkPackage)
{
    return Utils::filtered(existingKits(mcuTarget), [mcuTarget, qtForMCUsSdkPackage](Kit *kit) {
        return !kitIsUpToDate(kit, mcuTarget, qtForMCUsSdkPackage);
    });
}

QList<Kit *> kitsWithMismatchedDependencies(const McuTarget *mcuTarget)
{
    return Utils::filtered(existingKits(mcuTarget), [mcuTarget](Kit *kit) {
        const auto entries = Utils::NameValueDictionary(McuDependenciesKitAspect::configuration(kit));
        return Utils::anyOf(mcuTarget->packages(),
                            [&entries](const McuAbstractPackage *package) {
                                const QString cmakeVariableName = mapEnvVarToQul2xCmakeVar(package->environmentVariableName());
                                return !cmakeVariableName.isEmpty()
                                       && entries.value(cmakeVariableName)
                                              != package->path().toUserOutput();
                            });
    });
}

QList<Kit *> outdatedKits()
{
    return Utils::filtered(KitManager::kits(), [](Kit *kit) {
        return !kit->value(Constants::KIT_MCUTARGET_VENDOR_KEY).isNull()
               && kit->value(Constants::KIT_MCUTARGET_KITVERSION_KEY) != KIT_VERSION;
    });
}

void removeOutdatedKits()
{
    for (auto kit : outdatedKits())
        KitManager::deregisterKit(kit);
}

Kit *newKit(const McuTarget *mcuTarget, const McuAbstractPackage *qtForMCUsSdk)
{
    const auto init = [mcuTarget, qtForMCUsSdk](Kit *k) {
        KitGuard kitGuard(k);

        setKitProperties(kitName(mcuTarget), k, mcuTarget, qtForMCUsSdk->path());
        setKitDevice(k, mcuTarget);
        setKitToolchains(k, mcuTarget->toolChainPackage());
        setKitDebugger(k, mcuTarget->toolChainPackage());
        setKitEnvironment(k, mcuTarget, qtForMCUsSdk);
        setKitDependencies(k, mcuTarget, qtForMCUsSdk);
        setKitCMakeOptions(k, mcuTarget, qtForMCUsSdk->path());
        setKitQtVersionOptions(k);

        k->setup();
        k->fix();
    };

    return KitManager::registerKit(init);
}

QVersionNumber kitQulVersion(const Kit *kit)
{
    return QVersionNumber::fromString(
        kit->value(McuSupport::Constants::KIT_MCUTARGET_SDKVERSION_KEY).toString());
}

static FilePath kitDependencyPath(const Kit *kit, const QString &variableName)
{
    for (const NameValueItem &nameValueItem : EnvironmentKitAspect::environmentChanges(kit)) {
        if (nameValueItem.name == variableName)
            return FilePath::fromUserInput(nameValueItem.value);
    }
    return FilePath();
}

bool kitIsUpToDate(const Kit *kit,
                                  const McuTarget *mcuTarget,
                                  const McuAbstractPackage *qtForMCUsSdkPackage)
{
    return kitQulVersion(kit) == mcuTarget->qulVersion()
           && kitDependencyPath(kit, qtForMCUsSdkPackage->environmentVariableName()).toUserOutput()
                  == qtForMCUsSdkPackage->path().toUserOutput();
}

void createAutomaticKits()
{
    auto qtForMCUsPackage = Sdk::createQtForMCUsPackage();

    const auto createKits = [qtForMCUsPackage]() {
        if (McuSupportOptions::automaticKitCreationFromSettings()) {
            qtForMCUsPackage->updateStatus();
            if (!qtForMCUsPackage->isValidStatus()) {
                switch (qtForMCUsPackage->status()) {
                case McuAbstractPackage::Status::ValidPathInvalidPackage: {
                    printMessage(McuPackage::tr("Path %1 exists, but does not contain %2.")
                                     .arg(qtForMCUsPackage->path().toUserOutput(),
                                          qtForMCUsPackage->detectionPath().toUserOutput()),
                                 true);
                    break;
                }
                case McuAbstractPackage::Status::InvalidPath: {
                    printMessage(McuPackage::tr("Path %1 does not exist. Add the path in Tools > Options > "
                                    "Devices > MCU.")
                                     .arg(qtForMCUsPackage->path().toUserOutput()),
                                 true);
                    break;
                }
                case McuAbstractPackage::Status::EmptyPath: {
                    printMessage(McuPackage::tr("Missing %1. Add the path in Tools > Options > Devices > MCU.")
                                     .arg(qtForMCUsPackage->detectionPath().toUserOutput()),
                                 true);
                    return;
                }
                default:
                    break;
                }
                return;
            }

            if (CMakeProjectManager::CMakeToolManager::cmakeTools().isEmpty()) {
                printMessage(McuPackage::tr("No CMake tool was detected. Add a CMake tool in Tools > Options > "
                                "Kits > CMake."),
                             true);
                return;
            }

            FilePath dir = qtForMCUsPackage->path();
            McuSdkRepository repo;
            Sdk::targetsAndPackages(dir, &repo);

            bool needsUpgrade = false;
            for (const auto &target : qAsConst(repo.mcuTargets)) {
                // if kit already exists, skip
                if (!matchingKits(target, qtForMCUsPackage).empty())
                    continue;
                if (!upgradeableKits(target, qtForMCUsPackage).empty()) {
                    // if kit exists but wrong version/path
                    needsUpgrade = true;
                } else {
                    // if no kits for this target, create
                    if (target->isValid())
                        newKit(target, qtForMCUsPackage);
                    target->printPackageProblems();
                }
            }

            repo.deletePackagesAndTargets();

            if (needsUpgrade)
                McuSupportPlugin::askUserAboutMcuSupportKitsUpgrade();
        }
    };

    createKits();
    delete qtForMCUsPackage;
}

void upgradeKitsByCreatingNewPackage(UpgradeOption upgradeOption)
{
    if (upgradeOption == UpgradeOption::Ignore)
        return;

    auto qtForMCUsPackage = Sdk::createQtForMCUsPackage();

    auto dir = qtForMCUsPackage->path();
    McuSdkRepository repo;
    Sdk::targetsAndPackages(dir, &repo);

    for (const auto &target : qAsConst(repo.mcuTargets)) {
        if (!matchingKits(target, qtForMCUsPackage).empty())
            // already up-to-date
            continue;

        const auto kits = upgradeableKits(target, qtForMCUsPackage);
        if (!kits.empty()) {
            if (upgradeOption == UpgradeOption::Replace) {
                for (auto existingKit : kits)
                    KitManager::deregisterKit(existingKit);
            }

            if (target->isValid())
                newKit(target, qtForMCUsPackage);
            target->printPackageProblems();
        }
    }

    repo.deletePackagesAndTargets();
    delete qtForMCUsPackage;
}

void upgradeKitInPlace(ProjectExplorer::Kit *kit,
                                          const McuTarget *mcuTarget,
                                          const McuAbstractPackage *qtForMCUsSdk)
{
    setKitProperties(kitName(mcuTarget), kit, mcuTarget, qtForMCUsSdk->path());
    setKitEnvironment(kit, mcuTarget, qtForMCUsSdk);
    setKitDependencies(kit, mcuTarget, qtForMCUsSdk);
}

void fixKitsDependencies()
{
    auto qtForMCUsPackage = Sdk::createQtForMCUsPackage();

    FilePath dir = qtForMCUsPackage->path();
    McuSdkRepository repo;
    Sdk::targetsAndPackages(dir, &repo);
    for (const auto &target : qAsConst(repo.mcuTargets)) {
        if (target->isValid()) {
            for (auto *kit : kitsWithMismatchedDependencies(target)) {
                // ToDo: should not be environment, but cmake vars
                updateKitEnvironment(kit, target);
            }
        }
    }

    repo.deletePackagesAndTargets();
    delete qtForMCUsPackage;
}

/**
 * @brief Fix/update existing kits if needed
 */
void fixExistingKits()
{
    for (Kit *kit : KitManager::kits()) {
        if (!kit->hasValue(Constants::KIT_MCUTARGET_KITVERSION_KEY))
            continue;

        if (kit->isAutoDetected()) {
            kit->setAutoDetected(false);
        }

        // Check if the MCU kits are flagged as supplying a QtQuick import path, in order
        // to tell the QMLJS code-model that it won't need to add a fall-back import
        // path.
        const auto bringsQtQuickImportPath = QtSupport::SuppliesQtQuickImportPath::id();
        auto irrelevantAspects = kit->irrelevantAspects();
        if (!irrelevantAspects.contains(bringsQtQuickImportPath)) {
            irrelevantAspects.insert(bringsQtQuickImportPath);
            kit->setIrrelevantAspects(irrelevantAspects);
        }
        if (!kit->hasValue(bringsQtQuickImportPath)) {
            kit->setValue(bringsQtQuickImportPath, true);
        }

        // Check if the MCU kit supplies its import path.
        const auto kitQmlImportPath = QtSupport::KitQmlImportPath::id();
        if (!irrelevantAspects.contains(kitQmlImportPath)) {
            irrelevantAspects.insert(kitQmlImportPath);
            kit->setIrrelevantAspects(irrelevantAspects);
        }
        if (!kit->hasValue(kitQmlImportPath)) {
            auto config = CMakeProjectManager::CMakeConfigurationKitAspect::configuration(kit);
            for (const auto &cfgItem : qAsConst(config)) {
                if (cfgItem.key == "QUL_GENERATORS") {
                    auto idx = cfgItem.value.indexOf("/lib/cmake/Qul");
                    auto qulDir = cfgItem.value.left(idx);
                    kit->setValue(kitQmlImportPath, QVariant(qulDir + "/include/qul"));
                    break;
                }
            }
        }

        // Check if the MCU kit has the flag for merged header/qml-import paths set.
        const auto mergedPaths = QtSupport::KitHasMergedHeaderPathsWithQmlImportPaths::id();
        if (!irrelevantAspects.contains(mergedPaths)) {
            irrelevantAspects.insert(mergedPaths);
            kit->setIrrelevantAspects(irrelevantAspects);
        }
        if (!kit->value(mergedPaths, false).toBool()) {
            kit->setValue(mergedPaths, true);
        }
    }

    // Fix kit dependencies for known targets
    auto qtForMCUsPackage = Sdk::createQtForMCUsPackage();
    qtForMCUsPackage->updateStatus();
    if (qtForMCUsPackage->isValidStatus()) {
        FilePath dir = qtForMCUsPackage->path();
        McuSdkRepository repo;
        Sdk::targetsAndPackages(dir, &repo);
        for (const auto &target : qAsConst(repo.mcuTargets))
            for (auto kit : existingKits(target)) {
                if (McuDependenciesKitAspect::dependencies(kit).isEmpty()) {
                    setKitDependencies(kit, target, qtForMCUsPackage);
                }
            }

        repo.deletePackagesAndTargets();
    }
    delete qtForMCUsPackage;
}

} // namespace McuKitManager
} // namespace Internal
} // namespace McuSupport
